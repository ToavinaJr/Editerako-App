# Build

## Outils

- CMake ≥ 3.21, Ninja, compilateur C++20
- Qt 6 : Widgets, Network, Sql, Concurrent, **PdfWidgets**, **Svg / SvgWidgets**, Test, **LinguistTools**
- Windows : runtime **Edge WebView2** (généralement déjà installé) ; le SDK est téléchargé au configure dans `build/_deps/webview2`

Qt est trouvé via `CMAKE_PREFIX_PATH`, `QTDIR`, `qtpaths` / `qmake`, ou (Windows) `C:\Qt\<version>\mingw_64`.

## Scripts (recommandé)

Depuis la racine du dépôt :

| Action | Windows | Linux |
|---|---|---|
| Configure | `.\scripts\configure.ps1` | `./scripts/configure.sh` |
| Build | `.\scripts\build.ps1` | `./scripts/build.sh` |
| Lancer | `.\scripts\run.ps1` | `./scripts/run.sh` |
| Tests | `.\scripts\test.ps1` | `./scripts/test.sh` |
| Package | `.\scripts\package.ps1` | `./scripts/package.sh` |
| Nettoyer | `.\scripts\clean.ps1` | `./scripts/clean.sh` |

`-Config Release` / argument `Release` pour une build Release. Sanitizers : `-Config Asan` / `asan` (Linux/macOS). `.\scripts\run.ps1 -Build` compile puis lance.

Les scripts exigent `CMakeLists.txt` et `src/main.cpp` à la racine / sous `src/`.

## CMake

```
CMakeLists.txt          projet, find_package, qt_standard_project_setup, enable_testing
CMakePresets.json       debug / release / asan / ubsan / tsan (Ninja), CMAKE_EXPORT_COMPILE_COMMANDS
cmake/Warnings.cmake    -Wall ou /W4 ; -Werror / /WX si EDITERAKO_WARNINGS_AS_ERRORS (CI)
cmake/Sanitizers.cmake  ASan / UBSan / TSan (pas ASan+TSan, pas MinGW GCC)
cmake/Libraries.cmake   editerako_add_module() — libs statiques internes
cmake/TreeSitter.cmake  runtime + OBJECT libs par grammaire
cmake/Testing.cmake     editerako_add_test() + env sanitizers
cmake/Packaging.cmake   CPack ZIP/TGZ/DragNDrop, desktop/icon Linux
src/<module>/CMakeLists.txt  EditerakoCore, Editor, Syntax, Project, Terminal, Viewers, AI, Lsp, Scm, Tasks, Dap, Plugins
src/CMakeLists.txt      sous-modules, cible Editerako, qt_add_resources, qt_add_translations, install / deploy
tests/CMakeLists.txt    exécutables Qt Test (lient les libs de module)
```

Presets :

```powershell
cmake --preset debug
cmake --build --preset debug
ctest --preset debug --output-on-failure
```

Sanitizers (Linux/macOS, pas MinGW) : presets `asan` (ASan+UBSan) et `tsan`. Détail : [ci.md](ci.md).

```powershell
cmake --preset asan
cmake --build --preset asan
ctest --preset asan --output-on-failure
```

L’exécutable est toujours `${binaryDir}/Editerako(.exe)` (`CMAKE_RUNTIME_OUTPUT_DIRECTORY`), pas sous `src/`. Les tests vont dans `${binaryDir}/tests/`.

`qt_standard_project_setup()` active AUTOMOC / AUTOUIC. Les QSS sont embarqués par `qt_add_resources` (`:/editerako/themes/dark.qss`). Les catalogues Linguist (`translations/editerako_*.ts`) sont compilés en `.qm` sous `:/i18n` ; extraire les chaînes : `cmake --build --preset debug --target update_translations`. Pas de `MANUAL_FINALIZATION` (CMake ≥ 3.21).

`cmake --install` exécute `qt_generate_deploy_app_script` (DLL / plugins Qt). Packaging : `.\scripts\package.ps1` / `./scripts/package.sh`. Le lancement quotidien passe par `scripts/run.*`, qui ajoute `Qt/bin` au PATH.

## Qt Creator

Ouvrir le `CMakeLists.txt` racine. Kit Qt 6 **avec PdfWidgets et Svg**. Le dossier de kit (`build/Desktop_Qt_…`) est distinct de `build/debug` des scripts. Ne pas versionner les kits.

## Pièges

- **Lien « Permission denied »** : `Editerako.exe` tourne encore. Le tuer, relancer `build.ps1`.
- **PdfWidgets introuvable** : installer le composant Qt PDF (aqt : `-m qtpdf`).
- **Svg introuvable** : installer Qt SVG (aqt : `-m qtsvg`).
- **LinguistTools introuvable** : installer Qt Tools (aqt : `-m qttools`).
- **macOS `framework 'AGL' not found`** : le SDK 26 a retiré AGL. `cmake/Apple.cmake` ignore ce framework ; en CI, Qt ≥ 6.9.2.
- **Sources** : liste CMake explicite, pas de `file(GLOB)`. Un nouveau `.cpp` métier va dans `src/<module>/CMakeLists.txt`. L’exe (`app/`, `ui/`, `main.cpp`) s’ajoute dans `src/CMakeLists.txt`.
- **Sanitizers MinGW** : ASan/TSan ne sont pas supportés avec MinGW GCC. Utiliser Linux/macOS (`cmake --preset asan`).
