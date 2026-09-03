# Architecture

Editerako est **un exécutable Qt Widgets** (`Editerako`) qui lie des **bibliothèques statiques** internes, une par module. `MainWindow` est le **composition root** : il crée les services, connecte les signaux et affiche les dialogs. La logique métier vit dans les modules.

```
src/main.cpp + src/app/MainWindow + src/ui/     cible Editerako
        │
        ├── WorkspaceController  (project : Workspace + Explorer + Watcher)
        ├── SessionController    (core)
        ├── EditorManager / ViewerManager
        ├── TerminalPanel
        └── ChatWidget
```

```
src/main.cpp + src/app/ + src/ui/     cible Editerako
        │
        ├── EditerakoEditor  ← EditerakoSyntax ← tree_sitter
        │         ↑
        ├── EditerakoViewers
        ├── EditerakoProject
        ├── EditerakoTerminal
        ├── EditerakoAI
        └── EditerakoCore
```

Includes : `#include "module/Fichier.h"` avec `src/` comme chemin PUBLIC de chaque lib.

Graphe autorisé : **Core** n’a aucune dépendance vers Editor / Project / App. Les protocoles futurs (LSP, Git, Tasks) ne doivent pas dépendre de `MainWindow`. Détail CMake : [adr/0001-modular-cmake-targets.md](adr/0001-modular-cmake-targets.md).

## Modules

| Dossier | Cible CMake | Responsabilité |
|---|---|---|
| `app/` + `ui/` | `Editerako` (exe) | Composition root : menus, dialogs, D&D, câblage des signaux |
| `core/` | `EditerakoCore` | `AppSettings`, `SessionStore`, `SessionController`, `DiskChangePolicy`, `CommandRegistry`, `ThemeManager`, `DropPaths`, `AtomicFile`, logs |
| `editor/` | `EditerakoEditor` | `CodeEditor`, `EditorManager` (onglets), `EditorIo` / `EditorStyle` / `HighlighterSync` ; `features/` (gutter, ligne courante, multi-curseurs, swap de lignes) |
| `project/` | `EditerakoProject` | `Workspace`, `FileExplorer`, `FileWatcher`, `WorkspaceController` |
| `terminal/` | `EditerakoTerminal` | `Terminal`, `TerminalProcess`, `TerminalPanel` |
| `viewers/` | `EditerakoViewers` | `fileKindForPath`, `ViewerManager`, `PdfViewer`, `ImageViewer` |
| `ai/` | `EditerakoAI` | `AiProvider` / `GeminiProvider`, `ChatWidget`, `ChatRepository`, `ContextBuilder` |

Tree-sitter (runtime + grammaires C++ et HTML) est vendored dans `tree-sitter/` et compilé via `cmake/TreeSitter.cmake`. Seul `tree_sitter/api.h` est PUBLIC.

## Flux importants

**Ouvrir un fichier.** `MainWindow::openFileInEditor` → `ViewerManager::open` → `fileKindForPath` : texte (`EditorManager`), PDF, image, ou page « non supporté ».

**PDF.** Ne pas appeler `QPdfView::setDocument` avant `QPdfDocument::Status::Ready` (crash sinon). L’onglet est ajouté d’abord, le document ensuite.

**Session.** `SessionController` s’appuie sur `SessionStore` (`QSettings` org/app `Editerako`). Pendant un restore, `save` est un no-op (`RestoreGuard`). Restaure workspace, onglets existants, fichier actif, géométrie. Les *untitled* ne sont pas restaurés. Le dialogue d’accueil n’apparaît que s’il n’y a pas de session valide.

**Disque.** `WorkspaceController` câble `FileWatcher` (debounce 250 ms) et l’arbre. `EditorManager::aboutToSave` appelle `ignoreNextChange`. Un changement externe passe par `diskChangeAction()` ; `MainWindow` affiche les prompts.

**Terminal.** Cwd = dossier du fichier texte actif, sinon le workspace. Fermer le dernier onglet masque le panneau. `Ctrl+J` bascule la visibilité (après setup le panneau est masqué avec le flag « visible » : le premier `Ctrl+J` ne fait qu’aligner l’état).

**Éditeur.** `CodeEditor` délègue gutter, ligne courante, multi-curseurs et swap de lignes à `src/editor/features/`. `EditorManager` orchestre les onglets et les dialogs ; lecture via `readTextFile`, style via `EditorStyle`, highlighter via `HighlighterSync`. Détail : [adr/0003-editor-features.md](adr/0003-editor-features.md).

**Chat.** `GEMINI_API_KEY` (`.env` chargé au démarrage). Historique SQLite : `{projet}/.editerako/chat_history.db` (non versionné). Modèle : `gemini-2.0-flash-001`. `AiProvider::create` est le point d’extension pour un autre backend.

## Ce que MainWindow ne doit plus contenir

Pas de fan-out workspace (explorer + watcher), pas d’I/O session brute, pas de politique reload disque, pas de logique d’onglets terminal, pas de classification MIME. Déléguer à `WorkspaceController`, `SessionController`, `diskChangeAction()`, `TerminalPanel`, `fileKindForPath`. Détail : [adr/0002-mainwindow-composition-root.md](adr/0002-mainwindow-composition-root.md).
