# Documentation Editerako

Le [README](../README.MD) suffit pour compiler et lancer. Ici : comment le code est organisé, et comment l’étendre sans casser le build.

| Document | Contenu |
|---|---|
| [architecture.md](architecture.md) | Modules, rôle de `MainWindow`, flux fichiers / session / IA |
| [build.md](build.md) | CMake, presets, Qt Creator, déploiement, pièges Windows |
| [conventions.md](conventions.md) | Includes, nommage, QSS, CMake, journalisation |
| [testing.md](testing.md) | `ctest`, comment ajouter un test Qt Test |
| [settings.md](settings.md) | Priorité user/workspace, JSON, raccourcis |
| [adr/0001-modular-cmake-targets.md](adr/0001-modular-cmake-targets.md) | Cibles CMake par module |
| [adr/0002-mainwindow-composition-root.md](adr/0002-mainwindow-composition-root.md) | MainWindow composition root |
| [adr/0003-editor-features.md](adr/0003-editor-features.md) | Features extraits de `CodeEditor` / `EditorManager` |
| [adr/0004-document-encoding-eol.md](adr/0004-document-encoding-eol.md) | Encoding, BOM, EOL, modèle document |
| [adr/0005-settings-keybindings.md](adr/0005-settings-keybindings.md) | Settings UI et raccourcis |
| [adr/0006-command-palette-quick-open.md](adr/0006-command-palette-quick-open.md) | Command Palette et Quick Open |
| [adr/0007-workspace-search-explorer-ops.md](adr/0007-workspace-search-explorer-ops.md) | Search workspace et ops explorateur |
| [refactoring-baseline.md](refactoring-baseline.md) | Audit Phase 0 : architecture, features, dette, plan de migration |
