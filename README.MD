# Editerako

Éditeur de code / mini-IDE en **C++20** et **Qt 6 Widgets**.

Le projet est un prototype fonctionnel en cours de structuration architecturale. Les instructions ci-dessous décrivent **le build réellement supporté aujourd’hui**.

## Features

- Éditeur `QPlainTextEdit` personnalisé, numéros de ligne, multi-onglets, multi-curseurs, déplacement de lignes
- Coloration syntaxique Tree-sitter (grammaires **C++** et **HTML** actuellement)
- Ouverture / sauvegarde de fichiers, marqueur modifié, drag & drop
- Recherche, remplacement, regex, Go To Line
- Explorateur de fichiers, création de fichiers et de dossiers, ouverture de dossier
- Prévisualisation PDF et images
- Terminal intégré (plusieurs sessions, historique, autocomplete, découverte de commandes via `QProcess`)
- Chat IA Gemini, historique SQLite par projet (`.editerako/`, non versionné)

## Screenshots

![Editerako](captures/code_editor.png)

## Requirements

- **CMake** ≥ 3.21
- **Ninja**
- Compilateur C++20 (MinGW / GCC / Clang / MSVC)
- **Qt 6** avec les modules :
  - Widgets
  - Network
  - Sql
  - Concurrent
  - **PdfWidgets** (module Qt PDF — à installer séparément si `find_package` échoue)

Qt est localisé via `CMAKE_PREFIX_PATH`, `QTDIR`, `qtpaths` / `qmake` sur le `PATH`, ou (Windows) le layout de l’installateur `C:\Qt\<version>\mingw_64`.

Le thème (`dark` / `light`) est lu dans `QSettings` (organisation/application `Editerako`). La valeur par défaut est `dark`.

## Build Windows

Depuis la racine du dépôt, dans PowerShell :

```powershell
.\scripts\configure.ps1
.\scripts\build.ps1
.\scripts\run.ps1
```

Si l’exécution des scripts est bloquée : `Set-ExecutionPolicy -Scope CurrentUser RemoteSigned`.

Release :

```powershell
.\scripts\configure.ps1 -Config Release
.\scripts\build.ps1 -Config Release
.\scripts\run.ps1 -Config Release
```

Compiler puis lancer :

```powershell
.\scripts\run.ps1 -Build
```

Nettoyer uniquement `build/debug` (le kit Qt Creator n’est pas touché) :

```powershell
.\scripts\clean.ps1
```

Tout le dossier `build/` (y compris un kit Qt Creator) :

```powershell
.\scripts\clean.ps1 -All
```

CMake directement :

```powershell
cmake --preset debug
cmake --build --preset debug
```

### Qt Creator

Ouvrir `CMakeLists.txt`. Choisir un kit Qt 6 avec PdfWidgets. Le dossier de build de Qt Creator (par exemple `build/Desktop_Qt_…`) est distinct de `build/debug` utilisé par les scripts.

## Build Linux

```bash
./scripts/configure.sh
./scripts/build.sh
./scripts/run.sh
```

Release :

```bash
./scripts/configure.sh Release
./scripts/build.sh Release
./scripts/run.sh Release
```

Compiler puis lancer :

```bash
./scripts/run.sh --build
```

Nettoyer :

```bash
./scripts/clean.sh
./scripts/clean.sh --all
```

CMake :

```bash
cmake --preset debug
cmake --build --preset debug
```

Sur certaines distributions, installer le paquet **Qt PDF** en plus de Qt Widgets.

## AI configuration

Le chat lit `GEMINI_API_KEY` dans l’environnement. Un fichier `.env` à la racine (ignoré par Git) est chargé au démarrage s’il existe. Ne commitez jamais de clé.

## Project structure

```
CMakeLists.txt          projet, find_package Qt, qt_standard_project_setup
CMakePresets.json       presets debug / release (Ninja)
scripts/                configure, build, run, clean (Windows + Linux)
cmake/
  Warnings.cmake
  TreeSitter.cmake      bibliothèque vendor
src/CMakeLists.txt      cible Editerako, ressources, install / deploy
src/                    tout le code C++ / Qt
  main.cpp              point d'entrée
  app/                  MainWindow (orchestration)
  core/                 settings, thème, session, commandes
  editor/               éditeur, documents, find / go-to-line
  syntax/               Tree-sitter, highlighter
  project/              workspace, explorateur, file watcher
  terminal/             widget, process, panneau d'onglets
  viewers/              PDF, images
  ai/                   chat Gemini + persistance
  ui/                   helpers de layout / dialogues
resources/              thèmes QSS (dark / light), embarqués via qt_add_resources
tree-sitter/            runtime + grammaires C++ / HTML
```

## Testing

Aucun test automatisé n’est encore branché (`ctest` n’a pas de suite).

## Roadmap

Modulariser l’éditeur, Tree-sitter au niveau document, explorateur paresseux, terminal et chat IA découplés — sans casser les fonctionnalités listées ci-dessus.

## Contributing

Compiler via les scripts ou Qt Creator, ne pas versionner `build/`, `.env`, ni les artefacts CMake/MSVC générés (voir `.gitignore`).
