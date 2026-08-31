# Editerako

Éditeur de code / mini-IDE en **C++20** et **Qt 6 Widgets**.

Documentation détaillée : [docs/](docs/README.md) (architecture, build, conventions, tests).

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

- **CMake** ≥ 3.21, **Ninja**, compilateur C++20
- **Qt 6** : Widgets, Network, Sql, Concurrent, **PdfWidgets** (module Qt PDF), Test

Qt est localisé via `CMAKE_PREFIX_PATH`, `QTDIR`, `qtpaths` / `qmake`, ou (Windows) `C:\Qt\<version>\mingw_64`. Détail : [docs/build.md](docs/build.md).

## Build Windows

```powershell
.\scripts\configure.ps1
.\scripts\build.ps1
.\scripts\run.ps1
```

Si les scripts sont bloqués : `Set-ExecutionPolicy -Scope CurrentUser RemoteSigned`.

Release : ajouter `-Config Release`. Compiler puis lancer : `.\scripts\run.ps1 -Build`. Tests : `.\scripts\test.ps1`. Nettoyage : `.\scripts\clean.ps1` (`-All` pour tout `build/`).

CMake : `cmake --preset debug` puis `cmake --build --preset debug`.

Qt Creator : ouvrir `CMakeLists.txt`, kit Qt 6 **avec PdfWidgets**. Le kit (`build/Desktop_Qt_…`) est distinct de `build/debug`.

## Build Linux

```bash
./scripts/configure.sh
./scripts/build.sh
./scripts/run.sh
```

Release : argument `Release`. Compiler puis lancer : `./scripts/run.sh --build`. Tests : `./scripts/test.sh`. Sur certaines distros, installer **Qt PDF** en plus de Widgets.

## AI

Le chat lit `GEMINI_API_KEY`. Copier [`.env.example`](.env.example) vers `.env` à la racine (ignoré par Git). Ne jamais committer de clé.

## Contributing

Suivre [docs/conventions.md](docs/conventions.md). Ne pas versionner `build/`, `.env`, ni `{projet}/.editerako/`.
