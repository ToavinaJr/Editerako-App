# ADR 0021 — CI, sanitizers, packaging

- **Statut :** accepté
- **Date :** 2026-09-04
- **Phase :** 21

## Contexte

Le cahier des charges clôt le plan par une **matrice CI**, des **sanitizers**, `.clang-format` / `.clang-tidy`, et le **packaging** (ZIP Windows, AppImage Linux, DMG macOS). Un seul workflow macOS existait, sans CTest ni sanitizers. ASan et TSan ne peuvent pas partager le même exécutable. MSan est trop coûteux avec Qt non instrumenté. `_GLIBCXX_DEBUG` casse l’ABI vis-à-vis des bibliothèques Qt précompilées.

## Décision

- Options CMake `EDITERAKO_ENABLE_ASAN` / `UBSAN` / `TSAN` (`cmake/Sanitizers.cmake`). Presets `asan` (ASan+UBSan), `ubsan`, `tsan`, en plus de `debug` / `release`.
- ASan+TSan → `FATAL_ERROR`. MinGW GCC → sanitizers refusés (non fiables). TSan → pas Windows.
- CI (`.github/workflows/ci.yml`) : Ubuntu GCC debug+release, Clang ASan+UBSan, Clang TSan ; Windows MinGW Release ; macOS Release + CTest. `EDITERAKO_WARNINGS_AS_ERRORS` seulement en CI. clang-tidy / cppcheck en job **non bloquant**.
- Packaging Release : `scripts/package.ps1` (ZIP + `windeployqt` via le script d’install Qt), `scripts/package.sh` (TGZ, AppImage optionnel), DMG macOS (`create-dmg`). CPack ZIP/TGZ/DragNDrop.
- `.clang-format` et `.clang-tidy` documentent le style ; pas de reformat massif du dépôt dans cette phase.

Hors CI pour l’instant : MSan, Valgrind/Helgrind/DRD, CodeQL, `-Wconversion`.

## Conséquences

- Les sanitizers se lancent sur Linux/macOS : `./scripts/test.sh asan` / `tsan`.
- Les fuites Qt connues sont filtrées (`cmake/sanitizer-suppressions/`). TSan ignore `called_from_lib:libQt6*` (Pdf/Svg avec suffixe `.so`, sinon `libQt6Pdf` matche aussi PdfWidgets) et les faux positifs `condition_variable` de glibc 2.39. UBSan `-fsanitize=function` est coupé sur le runtime Tree-sitter (`foo()` C vs `void *(*)(void)`).
- Les artefacts d’installers sont publiés hors pull request (push `master` / `workflow_dispatch`).
