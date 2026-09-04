# Architecture

Editerako est **un exécutable Qt Widgets** (`Editerako`) qui lie des **bibliothèques statiques** internes, une par module. `MainWindow` est le **composition root** : il crée les services, connecte les signaux et affiche les dialogs. La logique métier vit dans les modules.

```
src/main.cpp + src/app/MainWindow + src/ui/     cible Editerako
        │
        ├── WorkspaceController  (project : Workspace + Explorer + Watcher + FileIndex)
        ├── SessionController    (core)
        ├── EditorManager / ViewerManager
        ├── BottomPanel (Problems + Terminal + Source Control + Diff)
        └── ChatWidget
```

```
src/main.cpp + src/app/ + src/ui/     cible Editerako
        │
        ├── EditerakoEditor  ← EditerakoSyntax ← tree_sitter
        │         ↑                    ↑
        │         └── EditerakoLsp ────┘
        ├── EditerakoViewers
        ├── EditerakoProject
        ├── EditerakoTerminal
        ├── EditerakoAI
        ├── EditerakoLsp
        ├── EditerakoScm
        ├── EditerakoTasks
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
| `terminal/` | `EditerakoTerminal` | `Terminal`, `TerminalProcess`, `ITerminalBackend` (`Process` / `Pty`), `AnsiSgr`, `ShellProfiles` |
| `viewers/` | `EditerakoViewers` | `fileKindForPath`, `ViewerManager`, `PdfViewer`, `ImageViewer` |
| `ai/` | `EditerakoAI` | `AiProvider` (Gemini / OpenAI / Anthropic / Ollama), chat **compte** (WebView2), `ChatWidget`, `ChatRepository`, `ContextBuilder` |
| `lsp/` | `EditerakoLsp` | JSON-RPC, client LSP, document sync, providers (pas d’UI). [adr/0010-lsp-infrastructure.md](adr/0010-lsp-infrastructure.md) |
| `scm/` | `EditerakoScm` | Provider Git CLI async, parsers porcelain, `TextDiff` (pas d’UI). [adr/0013-git-scm-diff.md](adr/0013-git-scm-diff.md) |
| `tasks/` | `EditerakoTasks` | `tasks.json`, CMake CLI (presets, configure/build/test/run), runner async, problem matcher. [adr/0014-tasks-cmake.md](adr/0014-tasks-cmake.md) |

`LspSession` (`src/app/`) démarre clangd pour C/C++ et alimente l’UI. Même classe, unités de compilation séparées : `LspSession.cpp` (cycle de vie, sync, diagnostics), `LspSessionFeatures.cpp` (completion / hover / signature), `LspSessionNavigation.cpp` (définition, références, rename, symboles). clangd reçoit `--compile-commands-dir` vers `lspCompileCommandsDir` (`build/debug`, …) pour résoudre les types du projet. Les lignes des pickers sont formatées par `lspLocationRows` / `lspSymbolRows` (`EditerakoLsp`). [adr/0011-clangd-editor-lsp.md](adr/0011-clangd-editor-lsp.md).

`MainWindow` reste le composition root, découpé par responsabilité (même classe) : `MainWindow.cpp` (cycle de vie / session), `MainWindowCommands.cpp` (menus), `MainWindowSetup.cpp` (câblage), `MainWindowWorkspace.cpp` (fichiers / disque / D&D), `MainWindowDialogs.cpp` (palettes). L’explorateur : menu dans `FileExplorerMenu.cpp`, icônes / badges dans `FileExplorerDecorations`.

Tree-sitter (runtime + grammaires réelles) est vendored dans `tree-sitter/` et compilé via `cmake/TreeSitter.cmake` (une OBJECT lib par grammaire). Seul `tree_sitter/api.h` est PUBLIC. Metadata : `LanguageDefinition` dans `LanguageRegistry`. Détail : [adr/0009-tree-sitter-multilang.md](adr/0009-tree-sitter-multilang.md).

## Flux importants

**Ouvrir un fichier.** `MainWindow::openFileInEditor` → `ViewerManager::open` → `fileKindForPath` : texte (`EditorManager`), PDF, image, ou page « non supporté ».

**PDF.** Ne pas appeler `QPdfView::setDocument` avant `QPdfDocument::Status::Ready` (crash sinon). L’onglet est ajouté d’abord, le document ensuite.

**Session.** `SessionController` s’appuie sur `SessionStore` (`QSettings` org/app `Editerako`). Pendant un restore, `save` est un no-op (`RestoreGuard`). Restaure workspace, onglets existants, fichier actif, géométrie. Les *untitled* ne sont pas restaurés. Le dialogue d’accueil n’apparaît que s’il n’y a pas de session valide.

**Disque.** `WorkspaceController` câble `FileWatcher` (debounce 250 ms) et l’arbre. `EditorManager::aboutToSave` appelle `ignoreNextChange`. Un changement externe passe par `diskChangeAction()` ; `MainWindow` affiche les prompts.

**Terminal.** Cwd = dossier du fichier texte actif, sinon le workspace. Le terminal vit dans `BottomPanel` (onglet Terminal). Fermer le dernier onglet terminal masque le panneau inférieur. `Ctrl+J` bascule la visibilité (après setup le panneau est masqué avec le flag « visible » : le premier `Ctrl+J` ne fait qu’aligner l’état). Commandes = one-shot via `ProcessTerminalBackend` par défaut ; PTY optionnel (`terminal/usePty`). `Ctrl+Shift+M` ouvre Problems. `Ctrl+Shift+G` ouvre Source Control. `Ctrl+Shift+B` lance le build. [adr/0012-problems-panel.md](adr/0012-problems-panel.md), [adr/0013-git-scm-diff.md](adr/0013-git-scm-diff.md), [adr/0014-tasks-cmake.md](adr/0014-tasks-cmake.md), [adr/0015-terminal-backend-pty.md](adr/0015-terminal-backend-pty.md).

**Git.** `GitCliProvider` détecte le dépôt du workspace, rafraîchit le statut hors UI, et alimente le panneau Source Control (Staged / Changes / Untracked) plus les badges de l’explorateur. Double-clic → diff unified. `file.compareWithDisk` compare le buffer éditeur au fichier disque.

**Tasks / CMake.** `TaskManager` charge `.editerako/tasks.json` et, si `CMakeLists.txt` est à la racine, expose Configure / Build / Clean / Test / Run via le CLI CMake (`--preset`). La sortie va dans l’onglet Output ; gcc/msvc alimentent Problems sans écraser clangd. `Ctrl+Shift+B` lance le build. [adr/0014-tasks-cmake.md](adr/0014-tasks-cmake.md).

**Éditeur.** `CodeEditor` délègue à `src/editor/features/` : gutter, ligne courante (+ matching de brackets), multi-curseurs, indent, commentaires, paires auto, commandes de lignes, occurrences. Swap de lignes : **même algorithme** `toPlainText().mid`. Menu Edit + `CommandRegistry`. Détail : [adr/0003-editor-features.md](adr/0003-editor-features.md), [adr/0008-editing-commands.md](adr/0008-editing-commands.md), [adr/0004-document-encoding-eol.md](adr/0004-document-encoding-eol.md).

**Settings.** Défaut < user QSettings < `{workspace}/.editerako/settings.json`. UI Préférences (`Ctrl+,`). Raccourcis via `KeybindingManager` (plus de `setShortcut` dans `MainWindow`). Détail : [settings.md](settings.md), [adr/0005-settings-keybindings.md](adr/0005-settings-keybindings.md).

**Navigation.** Command Palette (`Ctrl+Shift+P`) filtre `CommandRegistry`. Quick Open (`Ctrl+P`) utilise `WorkspaceFileIndex` (scan hors UI, exclusions respectées) et `fichier:ligne`. Search workspace (`Ctrl+Shift+F`) : texte/regex hors UI, preview, replace. Détail : [adr/0006-command-palette-quick-open.md](adr/0006-command-palette-quick-open.md), [adr/0007-workspace-search-explorer-ops.md](adr/0007-workspace-search-explorer-ops.md).

**Chat.** Par défaut **sign-in** (ChatGPT, Claude, Gemini Google, Copilot) dans le panneau : session du compte, pas de clé Gemini. WebView2 sous Windows (profil `%AppData%/Editerako/webview-profile`). Backends API optionnels (`GEMINI_API_KEY`, `OPENAI_API_KEY`, `ANTHROPIC_API_KEY`, Ollama local). Historique SQLite API : `{projet}/.editerako/chat_history.db`. [adr/0016-ai-account-chat.md](adr/0016-ai-account-chat.md).

## Ce que MainWindow ne doit plus contenir

Pas de fan-out workspace (explorer + watcher), pas d’I/O session brute, pas de politique reload disque, pas de logique d’onglets terminal, pas de classification MIME. Déléguer à `WorkspaceController`, `SessionController`, `diskChangeAction()`, `BottomPanel` / `TerminalPanel`, `fileKindForPath`. Détail : [adr/0002-mainwindow-composition-root.md](adr/0002-mainwindow-composition-root.md).
