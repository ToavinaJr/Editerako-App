# Conventions

## Fichiers et includes

- Code applicatif uniquement sous `src/`. PascalCase : `EditorManager.cpp`, pas `editormanager.cpp`.
- Garde d’include : `EDITERAKO_EDITORMANAGER_H`.
- Include interne : `"editor/EditorManager.h"` (chemin depuis `src/`). Features éditeur : `"editor/features/MultiCursorController.h"`. Jamais de chemin relatif `../`.
- Headers Qt et STL après les includes du projet. Forward-declare dans les `.h` quand un pointeur suffit (`MainWindow.h`).

## CMake

- Lister chaque `.cpp` / `.h` / `.ui` dans le `CMakeLists.txt` **du module** (`src/core/`, `src/editor/`, …). L’exe (`src/CMakeLists.txt`) ne contient que `app/`, `ui/` et `main.cpp`. Pas de GLOB.
- Cible app : `WIN32_EXECUTABLE ON` (pas de console). Cibles test : `OFF` (ctest doit voir la sortie).
- Modules : `editerako_add_module()` dans `cmake/Libraries.cmake` (`target_sources`, includes PUBLIC `src/`, `cxx_std_20`).
- Warnings via `editerako_enable_warnings()` ; Tree-sitter compile avec `-w` / `/W0`.
- Vendor : `cmake/TreeSitter.cmake` n’exporte en PUBLIC que `tree_sitter/api.h`. Une OBJECT lib par grammaire. Metadata langages : `LanguageDefinition` ([adr/0009-tree-sitter-multilang.md](adr/0009-tree-sitter-multilang.md)).
- Lier `PRIVATE`/`PUBLIC` explicitement. Ne pas ajouter `include_directories()` global.

## Qt

- `Q_OBJECT` + `tr()` pour tout texte UI.
- Raccourcis et menus via `CommandRegistry` (`file.save`, `edit.find`, `edit.toggleLineComment`, `edit.moveLineUp`, `view.terminal`, `workbench.commandPalette`, `workbench.quickOpen`, `workbench.search`, `workbench.problems`, `preferences.open`, `editor.gotoDefinition`, `editor.triggerSuggest`, …). Les séquences viennent de `KeybindingManager`, pas de `setShortcut` dans `MainWindow`. Tab / Shift+Tab d’indent restent dans `CodeEditor::keyPressEvent`. Completion Tab/Enter : `CompletionPopup` (filtre d’événements).
- `MainWindow` : composition root uniquement. Workspace → `WorkspaceController` ; session → `SessionController` ; reload disque → `diskChangeAction()` ; préférences → `SettingsDialog`.
- Éditeur : `CodeEditor` délègue à `editor/features/` ; I/O → `readTextFile` / `writeTextFile` (pas `QIODevice::Text`) ; style → `EditorStyle` ; highlighter → `HighlighterSync`. Ne pas « corriger » le swap de lignes (`toPlainText().mid`). Commandes d’édition : [adr/0008-editing-commands.md](adr/0008-editing-commands.md).
- Styles : `resources/themes/dark.qss` et `light.qss`. Object names stables pour le QSS : `terminalTabs`, `addTerminalButton`, `terminalCloseButton`, `pdfStatusLabel`, `editorStatusWidget`, `editorStatusProblems`, `fuzzyPickerDialog`, `commandPaletteDialog`, `quickOpenDialog`, `workspaceSearchDialog`, `searchResultsTree`, `searchPreview`, `completionPopup`, `hoverPopup`, `bottomPanel`, `bottomTabs`, `problemsPanel`, `problemsTree`.
- Pas de feuille de style inline sauf bulles HTML du chat (contenu dynamique).
- Thème : `AppSettings::themeId()` (`dark` / `light`), appliqué par `ThemeManager` au démarrage et depuis Préférences.

## Journalisation

Catégories dans `core/Logging.h` : `lcCore`, `lcEditor`, `lcSyntax`, `lcProject`, `lcTerminal`, `lcAi`, `lcViewer`, `lcLsp`. Utiliser `qCInfo` / `qCWarning` du module concerné, pas `qDebug()` nu.

## Données locales (ne pas committer)

| Chemin | Rôle |
|---|---|
| `.env` | `GEMINI_API_KEY=...` |
| `{projet}/.editerako/` | `chat_history.db`, `settings.json` (overlay workspace) |
| `build/` | artefacts CMake |

`QSettings` organisation/application : `Editerako`. Les tests de session / settings passent un `QSettings` fichier temporaire, pas le profil utilisateur. Overlay workspace : [settings.md](settings.md).

## Comportements à ne pas « corriger » à la légère

- PDF : `setDocument` seulement si le statut est `Ready`.
- Terminal : cwd = dossier du fichier actif, sinon workspace ; boutons × capturent le `Terminal*`, pas un index d’onglet.
- Premier `Ctrl+J` : voir [architecture.md](architecture.md).
- Fichiers texte : conserver encoding / BOM / EOL du disque. Buffer interne = LF. Détail : [adr/0004-document-encoding-eol.md](adr/0004-document-encoding-eol.md).
