# Documentation Editerako

Le [README](../README.MD) suffit pour compiler et lancer. Ici : comment le code est organisé, et comment l’étendre sans casser le build.

| Document | Contenu |
|---|---|
| [architecture.md](architecture.md) | Modules, rôle de `MainWindow`, flux fichiers / session / IA |
| [build.md](build.md) | CMake, presets, Qt Creator, déploiement, pièges Windows |
| [ci.md](ci.md) | Matrice GitHub Actions, sanitizers, packaging |
| [conventions.md](conventions.md) | Includes, nommage, QSS, CMake, journalisation |
| [testing.md](testing.md) | `ctest`, comment ajouter un test Qt Test |
| [settings.md](settings.md) | Priorité user/workspace, JSON, raccourcis |
| [plugins.md](plugins.md) | Plugins locaux `plugin.json` / `IPlugin` |
| [adr/0001-modular-cmake-targets.md](adr/0001-modular-cmake-targets.md) | Cibles CMake par module |
| [adr/0002-mainwindow-composition-root.md](adr/0002-mainwindow-composition-root.md) | MainWindow composition root |
| [adr/0003-editor-features.md](adr/0003-editor-features.md) | Features extraits de `CodeEditor` / `EditorManager` |
| [adr/0004-document-encoding-eol.md](adr/0004-document-encoding-eol.md) | Encoding, BOM, EOL, modèle document |
| [adr/0005-settings-keybindings.md](adr/0005-settings-keybindings.md) | Settings UI et raccourcis |
| [adr/0006-command-palette-quick-open.md](adr/0006-command-palette-quick-open.md) | Command Palette et Quick Open |
| [adr/0007-workspace-search-explorer-ops.md](adr/0007-workspace-search-explorer-ops.md) | Search workspace et ops explorateur |
| [adr/0008-editing-commands.md](adr/0008-editing-commands.md) | Indent, commentaires, paires, lignes, occurrences |
| [adr/0009-tree-sitter-multilang.md](adr/0009-tree-sitter-multilang.md) | Grammaires Tree-sitter réelles + LanguageDefinition |
| [adr/0010-lsp-infrastructure.md](adr/0010-lsp-infrastructure.md) | JSON-RPC LSP, client, tests mock |
| [adr/0011-clangd-editor-lsp.md](adr/0011-clangd-editor-lsp.md) | clangd, completion, hover, diagnostics, navigation |
| [adr/0012-problems-panel.md](adr/0012-problems-panel.md) | Problems Panel, BottomPanel, filtres Errors/Warnings |
| [adr/0013-git-scm-diff.md](adr/0013-git-scm-diff.md) | Git CLI async, panneau SCM, décorations, Diff |
| [adr/0014-tasks-cmake.md](adr/0014-tasks-cmake.md) | Tasks `.editerako/tasks.json`, CMake CLI, Output |
| [adr/0015-terminal-backend-pty.md](adr/0015-terminal-backend-pty.md) | `ITerminalBackend`, ConPTY/PTY, ANSI SGR |
| [adr/0016-ai-account-chat.md](adr/0016-ai-account-chat.md) | Sign-in compte + APIs OpenAI / Anthropic / Gemini / Ollama |
| [adr/0017-recovery-hot-exit.md](adr/0017-recovery-hot-exit.md) | Recovery, backups, Hot Exit |
| [adr/0018-editor-groups.md](adr/0018-editor-groups.md) | EditorArea / split |
| [adr/0019-dap-debugger.md](adr/0019-dap-debugger.md) | Client DAP, Debug panel |
| [adr/0020-plugin-system.md](adr/0020-plugin-system.md) | Plugins locaux |
| [adr/0021-ci-sanitizers-packaging.md](adr/0021-ci-sanitizers-packaging.md) | CI, sanitizers, packaging |
| [adr/0022-file-viewers.md](adr/0022-file-viewers.md) | Viewers Markdown, SVG, CSV |
| [adr/0023-code-folding.md](adr/0023-code-folding.md) | Code folding Tree-sitter |
| [adr/0024-i18n.md](adr/0024-i18n.md) | Internationalisation Linguist |
| [adr/0025-recent-workspaces.md](adr/0025-recent-workspaces.md) | Open Recent et welcome |
| [adr/0026-editor-tabs.md](adr/0026-editor-tabs.md) | Pin, preview, menu d’onglets |
| [refactoring-baseline.md](refactoring-baseline.md) | Audit Phase 0 : architecture, features, dette, plan de migration |
