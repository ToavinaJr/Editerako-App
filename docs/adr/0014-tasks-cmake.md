# ADR 0014 — Tasks et CMake CLI

- **Statut :** accepté
- **Date :** 2026-09-04
- **Phase :** 14

## Contexte

Le cahier des charges demande un module `src/tasks/`, un fichier `.editerako/tasks.json`, un runner, un problem matcher, et une intégration CMake (Configure / Build / Clean / Test / Run, presets, configuration, target) **sans remplacer CMake**.

## Décision

- `EditerakoTasks` : parseur `tasks.json`, inspection `CMakeLists.txt` / `CMakePresets.json`, construction des lignes de commande, `TaskRunner` (`QProcess` asynchrone, pas de `waitForFinished` sur le thread UI), `ProblemMatcher` gcc/msvc.
- Variables `${workspaceFolder}`, `${workspaceRoot}`, `${file}`.
- CMake : CLI uniquement (`cmake --preset`, `cmake --build --preset`, `ctest --preset`). Run lance l’exécutable du target en `startDetached`.
- UI dans le `BottomPanel` : onglets **Output** et **Tasks**. `Ctrl+Shift+B` = build (CMake ou première tâche « build »).
- Les diagnostics extraits n’écrasent pas ceux du LSP (`ProblemModel::setSourceProblems("task", …)`).

## Conséquences

- Pas de Widgets dans `src/tasks/`. CMake absent ou dossier sans `CMakeLists.txt` : panneau inactif, pas de crash.
- Un seul task à la fois. Side-by-side / kits CMake / `compile_commands` generation dédiée : hors phase.
- `.editerako/` reste gitignoré : `tasks.json` est local au workspace.
