# Architecture

Editerako est **un exécutable Qt Widgets** (`Editerako`) qui lie des **bibliothèques statiques** internes, une par module. `MainWindow` est le **composition root** : il crée les services, connecte les signaux et affiche les dialogs. La logique métier vit dans les modules.

```
src/main.cpp + src/app/MainWindow + src/ui/     cible Editerako
        │
        ├── WorkspaceController  (project : Workspace + Explorer + Watcher + FileIndex)
        ├── SessionController    (core)
        ├── EditorManager / ViewerManager
        ├── BottomPanel (Problems + Terminal + Source Control + Diff + Tasks + Debug)
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
        ├── EditerakoDap
        ├── EditerakoPlugins
        └── EditerakoCore
```

Includes : `#include "module/Fichier.h"` avec `src/` comme chemin PUBLIC de chaque lib.

Graphe autorisé : **Core** n’a aucune dépendance vers Editor / Project / App. `EditerakoLsp` ne dépend pas de `MainWindow` ni des widgets. Détail CMake : [adr/0001-modular-cmake-targets.md](adr/0001-modular-cmake-targets.md).

## Modules

| Dossier | Cible CMake | Responsabilité |
|---|---|---|
| `app/` + `ui/` | `Editerako` (exe) | Composition root : menus, dialogs (`SettingsDialog`, Command Palette, Quick Open, Search, `WelcomeDialog`), D&D, câblage des signaux |
| `core/` | `EditerakoCore` | `AppSettings` (user + overlay workspace), `TranslationLoader`, `RecentWorkspaces`, `KeybindingModel` / `KeybindingManager`, `FuzzyMatcher`, `SessionStore`, `SessionController`, `BackupService` / `RecoveryService`, `DiskChangePolicy`, `CommandRegistry`, `ThemeManager`, `DropPaths`, `AtomicFile`, `TextFileFormat`, logs |
| `editor/` | `EditerakoEditor` | `CodeEditor`, `EditorDocument`, `EditorArea` / `EditorGroup` (split, pin, preview), `TabOps`, `EditorManager`, `EditorIo` / `EditorStyle` / `HighlighterSync` / `EditorStatusWidget` ; `features/` (dont folding) |
| `project/` | `EditerakoProject` | `Workspace`, `FileExplorer`, `FileWatcher`, `WorkspaceFileIndex`, `WorkspacePath` / `WorkspaceOps`, `GitIgnore`, `WorkspaceSearch`, `WorkspaceController` |
| `terminal/` | `EditerakoTerminal` | `Terminal`, `TerminalProcess`, `ITerminalBackend` (`Process` / `Pty`), `AnsiSgr`, `ShellProfiles` |
| `viewers/` | `EditerakoViewers` | `fileKindForPath`, `ViewerManager`, PDF / image / SVG / CSV, aperçu Markdown |
| `ai/` | `EditerakoAI` | `AiProvider` (Gemini / OpenAI / Anthropic / Ollama), chat **compte** (WebView2), `ChatWidget`, `ChatRepository`, `ContextBuilder` |
| `lsp/` | `EditerakoLsp` | JSON-RPC, client LSP, document sync, providers (pas d’UI). [adr/0010-lsp-infrastructure.md](adr/0010-lsp-infrastructure.md) |
| `scm/` | `EditerakoScm` | Provider Git CLI async, parsers porcelain, `TextDiff` (pas d’UI). [adr/0013-git-scm-diff.md](adr/0013-git-scm-diff.md) |
| `tasks/` | `EditerakoTasks` | `tasks.json`, CMake CLI (presets, configure/build/test/run), runner async, problem matcher. [adr/0014-tasks-cmake.md](adr/0014-tasks-cmake.md) |
| `debug/` | `EditerakoDap` | Client DAP (pas LSP), `launch.json`, breakpoints. Pas d’UI. [adr/0019-dap-debugger.md](adr/0019-dap-debugger.md) |
| `plugins/` | `EditerakoPlugins` | `IPlugin` / `PluginManager` / `plugin.json`. Pas de marketplace. [adr/0020-plugin-system.md](adr/0020-plugin-system.md) |

`LspSession` (`src/app/`) démarre clangd pour C/C++ et alimente l’UI. Même classe, unités de compilation séparées : `LspSession.cpp` (cycle de vie, sync, diagnostics), `LspSessionFeatures.cpp` (completion / hover / signature), `LspSessionNavigation.cpp` (définition, références, rename, symboles). clangd reçoit `--compile-commands-dir` vers `lspCompileCommandsDir` (`build/debug`, …) pour résoudre les types du projet. Les lignes des pickers sont formatées par `lspLocationRows` / `lspSymbolRows` (`EditerakoLsp`). [adr/0011-clangd-editor-lsp.md](adr/0011-clangd-editor-lsp.md).

`DebugSession` (`src/app/`) spawn l’adaptateur DAP (`gdb --interpreter=dap` ou `lldb-dap`), synchronise les breakpoints, et alimente l’onglet Debug (stack, variables, console). [adr/0019-dap-debugger.md](adr/0019-dap-debugger.md).

`PluginManager` (`src/plugins/`, créé par `MainWindow`) scanne `%AppData%/Editerako/plugins` et `{workspace}/.editerako/plugins`. [adr/0020-plugin-system.md](adr/0020-plugin-system.md), [plugins.md](plugins.md).

`MainWindow` reste le composition root, découpé par responsabilité (même classe) : `MainWindow.cpp` (cycle de vie / session), `MainWindowCommands.cpp` (menus), `MainWindowSetup.cpp` (câblage), `MainWindowWorkspace.cpp` (fichiers / disque / D&D), `MainWindowDialogs.cpp` (palettes). L’explorateur : menu dans `FileExplorerMenu.cpp`, icônes / badges dans `FileExplorerDecorations`.

Tree-sitter (runtime + grammaires réelles) est vendored dans `tree-sitter/` et compilé via `cmake/TreeSitter.cmake` (une OBJECT lib par grammaire). Seul `tree_sitter/api.h` est PUBLIC. Metadata : `LanguageDefinition` dans `LanguageRegistry`. Détail : [adr/0009-tree-sitter-multilang.md](adr/0009-tree-sitter-multilang.md).

## Flux importants

**Ouvrir un fichier.** `MainWindow::openFileInEditor` → `ViewerManager::open` → `fileKindForPath` : texte (`EditorManager`), PDF, image, SVG, CSV, ou page « non supporté ». Markdown et JSON restent du texte ; aperçu Markdown : `file.markdownPreview`. [adr/0022-file-viewers.md](adr/0022-file-viewers.md).

**PDF.** Ne pas appeler `QPdfView::setDocument` avant `QPdfDocument::Status::Ready` (crash sinon). L’onglet est ajouté d’abord, le document ensuite.

**Session.** `SessionController` s’appuie sur `SessionStore` (`QSettings` org/app `Editerako`). Pendant un restore, `save` est un no-op (`RestoreGuard`). Restaure workspace, onglets existants, fichier actif, géométrie. Les *untitled* dirty et les buffers non sauvés sont restaurés par `RecoveryService` (Hot Exit / crash), pas par la session. Le dialogue d’accueil (`WelcomeDialog`) n’apparaît que s’il n’y a ni session workspace ni snapshot de recovery ; il liste les récents (`RecentWorkspaces`, File > Open Recent). [adr/0017-recovery-hot-exit.md](adr/0017-recovery-hot-exit.md), [adr/0025-recent-workspaces.md](adr/0025-recent-workspaces.md).

**Disque.** `WorkspaceController` câble `FileWatcher` (debounce 250 ms) et l’arbre. `EditorManager::aboutToSave` appelle `ignoreNextChange`. Un changement externe passe par `diskChangeAction()` ; `MainWindow` affiche les prompts.

**Terminal.** Cwd = dossier du fichier texte actif, sinon le workspace. Le terminal vit dans `BottomPanel` (onglet Terminal). Fermer le dernier onglet terminal masque le panneau inférieur. `Ctrl+J` bascule la visibilité (après setup le panneau est masqué avec le flag « visible » : le premier `Ctrl+J` ne fait qu’aligner l’état). Commandes = one-shot via `ProcessTerminalBackend` par défaut ; PTY optionnel (`terminal/usePty`). `Ctrl+Shift+M` ouvre Problems. `Ctrl+Shift+G` ouvre Source Control. `Ctrl+Shift+B` lance le build. `Ctrl+Shift+Y` ouvre Debug (F5 démarre / continue). [adr/0012-problems-panel.md](adr/0012-problems-panel.md), [adr/0013-git-scm-diff.md](adr/0013-git-scm-diff.md), [adr/0014-tasks-cmake.md](adr/0014-tasks-cmake.md), [adr/0015-terminal-backend-pty.md](adr/0015-terminal-backend-pty.md), [adr/0019-dap-debugger.md](adr/0019-dap-debugger.md).

**Git.** `GitCliProvider` détecte le dépôt du workspace, rafraîchit le statut hors UI, et alimente le panneau Source Control (Staged / Changes / Untracked) plus les badges de l’explorateur. Double-clic → diff unified. `file.compareWithDisk` compare le buffer éditeur au fichier disque.

**Tasks / CMake.** `TaskManager` charge `.editerako/tasks.json` et, si `CMakeLists.txt` est à la racine, expose Configure / Build / Clean / Test / Run via le CLI CMake (`--preset`). La sortie va dans l’onglet Output ; gcc/msvc alimentent Problems sans écraser clangd. `Ctrl+Shift+B` lance le build. [adr/0014-tasks-cmake.md](adr/0014-tasks-cmake.md).

**Éditeur.** `EditorArea` contient un ou plusieurs `EditorGroup` (`QTabWidget`). Sans split, un seul groupe — même UX qu’avant. Split Right / Down : vues partagées du `QTextDocument` (texte) ou second viewer. Onglets : pin à gauche, preview optionnel (clic explorateur), Close to Right / Close Saved, Copy Path / Reveal. [adr/0018-editor-groups.md](adr/0018-editor-groups.md), [adr/0026-editor-tabs.md](adr/0026-editor-tabs.md). `CodeEditor` délègue à `src/editor/features/` : gutter, ligne courante (+ matching de brackets), multi-curseurs, indent, commentaires, paires auto, commandes de lignes, occurrences, **folding Tree-sitter**. Swap de lignes : **même algorithme** `toPlainText().mid`. Menu Edit + `CommandRegistry`. Détail : [adr/0003-editor-features.md](adr/0003-editor-features.md), [adr/0008-editing-commands.md](adr/0008-editing-commands.md), [adr/0004-document-encoding-eol.md](adr/0004-document-encoding-eol.md), [adr/0023-code-folding.md](adr/0023-code-folding.md).

**Settings.** Défaut < user QSettings < `{workspace}/.editerako/settings.json`. UI Préférences (`Ctrl+,`). Raccourcis via `KeybindingManager` (plus de `setShortcut` dans `MainWindow`). Langue : `ui/language` (`en` / `fr` / vide = système), catalogues Linguist `:/i18n`. Détail : [settings.md](settings.md), [adr/0005-settings-keybindings.md](adr/0005-settings-keybindings.md), [adr/0024-i18n.md](adr/0024-i18n.md).

**Navigation.** Command Palette (`Ctrl+Shift+P`) filtre `CommandRegistry`. Quick Open (`Ctrl+P`) utilise `WorkspaceFileIndex` (scan hors UI, exclusions respectées) et `fichier:ligne`. Search workspace (`Ctrl+Shift+F`) : texte/regex hors UI, preview, replace. Détail : [adr/0006-command-palette-quick-open.md](adr/0006-command-palette-quick-open.md), [adr/0007-workspace-search-explorer-ops.md](adr/0007-workspace-search-explorer-ops.md).

**Chat.** Par défaut **sign-in** (ChatGPT, Claude, Gemini Google, Copilot) dans le panneau : session du compte, pas de clé Gemini. WebView2 sous Windows (profil `%AppData%/Editerako/webview-profile`). Backends API optionnels (`GEMINI_API_KEY`, `OPENAI_API_KEY`, `ANTHROPIC_API_KEY`, Ollama local). Historique SQLite API : `{projet}/.editerako/chat_history.db`. [adr/0016-ai-account-chat.md](adr/0016-ai-account-chat.md).

## Ce que MainWindow ne doit plus contenir

Pas de fan-out workspace (explorer + watcher), pas d’I/O session brute, pas de politique reload disque, pas de logique d’onglets terminal, pas de classification MIME. Déléguer à `WorkspaceController`, `SessionController`, `diskChangeAction()`, `BottomPanel` / `TerminalPanel`, `fileKindForPath`. Détail : [adr/0002-mainwindow-composition-root.md](adr/0002-mainwindow-composition-root.md).
