# Architecture

Editerako est **un exécutable Qt Widgets** (`Editerako`) qui lie des **bibliothèques statiques** internes, une par module. `MainWindow` orchestre ; la logique métier vit dans les modules.

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
| `app/` + `ui/` | `Editerako` (exe) | Fenêtre principale, menus, drag-and-drop, helpers de layout |
| `core/` | `EditerakoCore` | `AppSettings`, `SessionStore`, `CommandRegistry`, `ThemeManager`, `DropPaths`, `AtomicFile`, logs |
| `editor/` | `EditerakoEditor` | `CodeEditor`, `EditorDocument`, `EditorManager`, find / go-to-line |
| `syntax/` | `EditerakoSyntax` | `LanguageRegistry` (C++ / HTML), `TreeSitterDocument`, `SyntaxHighlighter` |
| `project/` | `EditerakoProject` | `Workspace`, `FileExplorer`, `FileWatcher` |
| `terminal/` | `EditerakoTerminal` | `Terminal`, `TerminalProcess`, `TerminalPanel` |
| `viewers/` | `EditerakoViewers` | `fileKindForPath`, `ViewerManager`, `PdfViewer`, `ImageViewer` |
| `ai/` | `EditerakoAI` | `AiProvider` / `GeminiProvider`, `ChatWidget`, `ChatRepository`, `ContextBuilder` |

Tree-sitter (runtime + grammaires C++ et HTML) est vendored dans `tree-sitter/` et compilé via `cmake/TreeSitter.cmake`. Seul `tree_sitter/api.h` est PUBLIC.

## Flux importants

**Ouvrir un fichier.** `MainWindow::openFileInEditor` → `ViewerManager::open` → `fileKindForPath` : texte (`EditorManager`), PDF, image, ou page « non supporté ».

**PDF.** Ne pas appeler `QPdfView::setDocument` avant `QPdfDocument::Status::Ready` (crash sinon). L’onglet est ajouté d’abord, le document ensuite.

**Session.** `SessionStore` lit/écrit `QSettings` (org/app `Editerako`). Restaure workspace, onglets existants, fichier actif, géométrie. Les *untitled* ne sont pas restaurés. Le dialogue d’accueil n’apparaît que s’il n’y a pas de session valide.

**Disque.** `FileWatcher` surveille la racine (debounce 250 ms) et les fichiers ouverts. `EditorManager::aboutToSave` appelle `ignoreNextChange` pour ne pas recharger nos propres sauvegardes.

**Terminal.** Cwd = dossier du fichier texte actif, sinon le workspace. Fermer le dernier onglet masque le panneau. `Ctrl+J` bascule la visibilité (après setup le panneau est masqué avec le flag « visible » : le premier `Ctrl+J` ne fait qu’aligner l’état).

**Chat.** `GEMINI_API_KEY` (`.env` chargé au démarrage). Historique SQLite : `{projet}/.editerako/chat_history.db` (non versionné). Modèle : `gemini-2.0-flash-001`. `AiProvider::create` est le point d’extension pour un autre backend.

## Ce que MainWindow ne doit plus contenir

Pas de logique d’onglets terminal, pas d’I/O fichier brut, pas de classification MIME. Déléguer à `TerminalPanel`, `Workspace`, `fileKindForPath`.
