# Conventions

## Fichiers et includes

- Code applicatif uniquement sous `src/`. PascalCase : `EditorManager.cpp`, pas `editormanager.cpp`.
- Garde d’include : `EDITERAKO_EDITORMANAGER_H`.
- Include interne : `"editor/EditorManager.h"` (chemin depuis `src/`). Jamais de chemin relatif `../`.
- Headers Qt et STL après les includes du projet. Forward-declare dans les `.h` quand un pointeur suffit (`MainWindow.h`).

## CMake

- Lister chaque `.cpp` / `.h` / `.ui` dans `src/CMakeLists.txt`. Pas de GLOB.
- Cible app : `WIN32_EXECUTABLE ON` (pas de console). Cibles test : `OFF` (ctest doit voir la sortie).
- Warnings via `editerako_enable_warnings()` ; Tree-sitter compile avec `-w` / `/W0`.
- Vendor : `cmake/TreeSitter.cmake` n’exporte en PUBLIC que `tree_sitter/api.h`.

## Qt

- `Q_OBJECT` + `tr()` pour tout texte UI.
- Raccourcis et menus via `CommandRegistry` (`file.save`, `edit.find`, `view.terminal`, …).
- Styles : `resources/themes/dark.qss` et `light.qss`. Object names stables pour le QSS : `terminalTabs`, `addTerminalButton`, `terminalCloseButton`, `pdfStatusLabel`.
- Pas de feuille de style inline sauf bulles HTML du chat (contenu dynamique).
- Thème : `AppSettings::themeId()` (`dark` / `light`), appliqué par `ThemeManager` au démarrage.

## Journalisation

Catégories dans `core/Logging.h` : `lcCore`, `lcEditor`, `lcSyntax`, `lcProject`, `lcTerminal`, `lcAi`, `lcViewer`. Utiliser `qCInfo` / `qCWarning` du module concerné, pas `qDebug()` nu.

## Données locales (ne pas committer)

| Chemin | Rôle |
|---|---|
| `.env` | `GEMINI_API_KEY=...` |
| `{projet}/.editerako/` | `chat_history.db` |
| `build/` | artefacts CMake |

`QSettings` organisation/application : `Editerako`. Les tests de session passent un `QSettings` fichier temporaire, pas le profil utilisateur.

## Comportements à ne pas « corriger » à la légère

- PDF : `setDocument` seulement si le statut est `Ready`.
- Terminal : cwd = dossier du fichier actif, sinon workspace ; boutons × capturent le `Terminal*`, pas un index d’onglet.
- Premier `Ctrl+J` : voir [architecture.md](architecture.md).
