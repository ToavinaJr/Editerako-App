# Architecture

Editerako est **un exécutable Qt Widgets** (`Editerako`) qui lie des **bibliothèques statiques** internes, une par module. `MainWindow` est le **composition root** : il crée les services, connecte les signaux et affiche les dialogs. La logique métier vit dans les modules.

```
src/main.cpp + src/app/MainWindow + src/ui/     cible Editerako
        │
        ├── WorkspaceController  (project : Workspace + Explorer + Watcher + FileIndex)
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
        ├── EditerakoLsp
        └── EditerakoCore
```

Includes : `#include "module/Fichier.h"` avec `src/` comme chemin PUBLIC de chaque lib.

Graphe autorisé : **Core** n’a aucune dépendance vers Editor / Project / App. `EditerakoLsp` ne dépend pas de `MainWindow` ni des widgets. Détail CMake : [adr/0001-modular-cmake-targets.md](adr/0001-modular-cmake-targets.md).

## Modules

| Dossier | Cible CMake | Responsabilité |
|---|---|---|
| `app/` + `ui/` | `Editerako` (exe) | Composition root : menus, dialogs (`SettingsDialog`, Command Palette, Quick Open, Search), D&D, câblage des signaux |
| `core/` | `EditerakoCore` | `AppSettings` (user + overlay workspace), `KeybindingModel` / `KeybindingManager`, `FuzzyMatcher`, `SessionStore`, `SessionController`, `DiskChangePolicy`, `CommandRegistry`, `ThemeManager`, `DropPaths`, `AtomicFile`, `TextFileFormat`, logs |
| `editor/` | `EditerakoEditor` | `CodeEditor`, `EditorDocument` (path, encoding, EOL, language, version, caret), `EditorManager`, `EditorIo` / `EditorStyle` / `HighlighterSync` / `EditorStatusWidget` ; `features/` |
| `project/` | `EditerakoProject` | `Workspace`, `FileExplorer`, `FileWatcher`, `WorkspaceFileIndex`, `WorkspacePath` / `WorkspaceOps`, `GitIgnore`, `WorkspaceSearch`, `WorkspaceController` |
| `terminal/` | `EditerakoTerminal` | `Terminal`, `TerminalProcess`, `TerminalPanel` |
| `viewers/` | `EditerakoViewers` | `fileKindForPath`, `ViewerManager`, `PdfViewer`, `ImageViewer` |
| `ai/` | `EditerakoAI` | `AiProvider` / `GeminiProvider`, `ChatWidget`, `ChatRepository`, `ContextBuilder` |
| `lsp/` | `EditerakoLsp` | JSON-RPC, client LSP, document sync, providers (pas d’UI). [adr/0010-lsp-infrastructure.md](adr/0010-lsp-infrastructure.md) |

`LspSession` (`src/app/`) démarre clangd pour C/C++, synchronise les buffers, et pilote completion / hover / diagnostics / navigation. [adr/0011-clangd-editor-lsp.md](adr/0011-clangd-editor-lsp.md).

Tree-sitter (runtime + grammaires réelles) est vendored dans `tree-sitter/` et compilé via `cmake/TreeSitter.cmake` (une OBJECT lib par grammaire). Seul `tree_sitter/api.h` est PUBLIC. Metadata : `LanguageDefinition` dans `LanguageRegistry`. Détail : [adr/0009-tree-sitter-multilang.md](adr/0009-tree-sitter-multilang.md).

## Flux importants

**Ouvrir un fichier.** `MainWindow::openFileInEditor` → `ViewerManager::open` → `fileKindForPath` : texte (`EditorManager`), PDF, image, ou page « non supporté ».

**PDF.** Ne pas appeler `QPdfView::setDocument` avant `QPdfDocument::Status::Ready` (crash sinon). L’onglet est ajouté d’abord, le document ensuite.

**Session.** `SessionController` s’appuie sur `SessionStore` (`QSettings` org/app `Editerako`). Pendant un restore, `save` est un no-op (`RestoreGuard`). Restaure workspace, onglets existants, fichier actif, géométrie. Les *untitled* ne sont pas restaurés. Le dialogue d’accueil n’apparaît que s’il n’y a pas de session valide.

**Disque.** `WorkspaceController` câble `FileWatcher` (debounce 250 ms) et l’arbre. `EditorManager::aboutToSave` appelle `ignoreNextChange`. Un changement externe passe par `diskChangeAction()` ; `MainWindow` affiche les prompts.

**Terminal.** Cwd = dossier du fichier texte actif, sinon le workspace. Fermer le dernier onglet masque le panneau. `Ctrl+J` bascule la visibilité (après setup le panneau est masqué avec le flag « visible » : le premier `Ctrl+J` ne fait qu’aligner l’état).

**Éditeur.** `CodeEditor` délègue à `src/editor/features/` : gutter, ligne courante (+ matching de brackets), multi-curseurs, indent, commentaires, paires auto, commandes de lignes, occurrences. Swap de lignes : **même algorithme** `toPlainText().mid`. Menu Edit + `CommandRegistry`. Détail : [adr/0003-editor-features.md](adr/0003-editor-features.md), [adr/0008-editing-commands.md](adr/0008-editing-commands.md), [adr/0004-document-encoding-eol.md](adr/0004-document-encoding-eol.md).

**Settings.** Défaut < user QSettings < `{workspace}/.editerako/settings.json`. UI Préférences (`Ctrl+,`). Raccourcis via `KeybindingManager` (plus de `setShortcut` dans `MainWindow`). Détail : [settings.md](settings.md), [adr/0005-settings-keybindings.md](adr/0005-settings-keybindings.md).

**Navigation.** Command Palette (`Ctrl+Shift+P`) filtre `CommandRegistry`. Quick Open (`Ctrl+P`) utilise `WorkspaceFileIndex` (scan hors UI, exclusions respectées) et `fichier:ligne`. Search workspace (`Ctrl+Shift+F`) : texte/regex hors UI, preview, replace. Détail : [adr/0006-command-palette-quick-open.md](adr/0006-command-palette-quick-open.md), [adr/0007-workspace-search-explorer-ops.md](adr/0007-workspace-search-explorer-ops.md).

**Chat.** `GEMINI_API_KEY` (`.env` chargé au démarrage). Historique SQLite : `{projet}/.editerako/chat_history.db` (non versionné). Modèle / endpoint : `AppSettings` (`gemini-2.0-flash-001` par défaut). `AiProvider::create` est le point d’extension pour un autre backend.

## Ce que MainWindow ne doit plus contenir

Pas de fan-out workspace (explorer + watcher), pas d’I/O session brute, pas de politique reload disque, pas de logique d’onglets terminal, pas de classification MIME. Déléguer à `WorkspaceController`, `SessionController`, `diskChangeAction()`, `TerminalPanel`, `fileKindForPath`. Détail : [adr/0002-mainwindow-composition-root.md](adr/0002-mainwindow-composition-root.md).
