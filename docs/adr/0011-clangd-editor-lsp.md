# ADR 0011 — clangd, completion, hover, diagnostics, navigation

- **Statut :** accepté
- **Date :** 2026-09-03
- **Phase :** 11

## Contexte

La phase 10 a livré le client LSP sans serveur ni UI. Le cahier des charges demande clangd pour C/C++, une popup de completion **découplée du protocole**, un hover Markdown, des underlines/gutter de diagnostics, et la navigation (définition, références, rename, symboles). Le Problems Panel reste en phase 12.

## Décision

- `LspSession` (`src/app/`) est le pont composition-root : démarre clangd à la demande, synchronise les documents, alimente l’UI. `EditerakoLsp` reste sans Widgets.
- Completion : `CompletionModel` / `CompletionPopup` / `CompletionItem` génériques. Mapping LSP → modèle dans `LspSession`.
- Diagnostics éditeur : `EditorDiagnostic` + `DiagnosticMarkup` (WaveUnderline + pastille gutter). Problems Panel : [adr/0012-problems-panel.md](adr/0012-problems-panel.md).
- clangd : spec `clangd` + `--offset-encoding=utf-16`. `initialize` seulement après `QProcess::started`. Binaire absent : message status bar, pas de crash.
- Raccourcis : `F12`, `Shift+F12`, `F2`, `Ctrl+Space`, `Ctrl+Shift+O`, `Ctrl+T`.

## Conséquences

- Un `.c`/`.cpp`/`.h` ouvert tente clangd. Les autres langages ignorent le LSP.
- `languageServer` de `LanguageDefinition` est consommé (`clangd`).
- codeAction UI non livrée. Snippets LSP non annoncés.
