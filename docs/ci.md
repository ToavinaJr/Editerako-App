# CI, sanitizers, packaging

Détail des décisions : [adr/0021-ci-sanitizers-packaging.md](adr/0021-ci-sanitizers-packaging.md).

## Presets

| Preset | Rôle |
|---|---|
| `debug` | développement quotidien |
| `release` | optimisé, packaging |
| `asan` | AddressSanitizer **+** UndefinedBehaviorSanitizer (`-O1`) |
| `ubsan` | UBSan seul |
| `tsan` | ThreadSanitizer — **pas** combinable avec ASan |

```bash
cmake --preset asan
cmake --build --preset asan
ctest --preset asan --output-on-failure
```

Équivalent scripts : `./scripts/test.sh asan` (Linux/macOS). Sous **MinGW GCC**, ASan/TSan sont refusés au configure.

Ne pas mélanger ASan et TSan dans le même build.

Tree-sitter : le runtime appelle les scanners C `foo()` comme `void *(*)(void)` ; `-fsanitize=function` est désactivé sur la cible `tree_sitter`. TSan : Qt et les `condition_variable` glibc 2.39 (Ubuntu 24.04) sont filtrés dans `cmake/sanitizer-suppressions/tsan.supp`. Les `called_from_lib` Pdf/Svg portent le suffixe `.so` pour ne pas matcher deux bibliothèques (`libQt6Pdf` ∩ `libQt6PdfWidgets`).

## Outils volontairement absents de la CI

| Outil | Pourquoi |
|---|---|
| MSan | Qt et les libs système devraient être instrumentés |
| `_GLIBCXX_DEBUG` | ABI incompatible avec Qt précompilé |
| Valgrind / Helgrind / DRD | utile en local Linux, trop lent pour chaque PR |
| `-Wconversion` / `-Wold-style-cast` | trop de bruit sur la base existante (`-Wall -Wextra -Wpedantic` + `-Werror` en CI) |

## GitHub Actions

Workflow [`.github/workflows/ci.yml`](../.github/workflows/ci.yml) :

```
PR / push master
 ├── Linux GCC Debug + CTest + -Werror
 ├── Linux GCC Release + CTest + -Werror
 ├── Linux Clang ASan+UBSan + CTest
 ├── Linux Clang TSan + CTest
 ├── Windows MinGW Release + CTest
 ├── macOS Release + CTest
 └── clang-tidy + cppcheck (rapport, ne bloque pas)
```

Hors pull request : ZIP Windows, TGZ/AppImage Linux, DMG macOS en artefacts.

Qt CI : **6.9.2** + module extra `qtpdf`. SVG et Linguist (`qtsvg`, `qttools`) sont des archives de base aqt : ne pas les passer en `-m` (sinon aqt exit 1). Linux : arch `linux_gcc_64` (plus `gcc_64` depuis Qt 6.7). Tests : `QT_QPA_PLATFORM=offscreen`.

## clang-format / clang-tidy

Fichiers racine `.clang-format` et `.clang-tidy`. Ne pas reformater tout `src/` d’un coup. En local :

```bash
clang-format -i src/foo/Bar.cpp
./scripts/tidy.sh debug
```

`tidy.sh` ne parcourt que `<racine>/src/` (modules applicatifs). Pas Tree-sitter, pas `tests/` (fichiers `.moc` / AUTOMOC). Le job CI compile le preset debug avant tidy pour que UIC/MOC existent. `clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling` (Annex K `*_s`) est désactivé : bruit sur le C POSIX et le vendor.

## Packaging

| Plateforme | Commande | Sortie |
|---|---|---|
| Windows | `.\scripts\package.ps1` | `dist/Editerako-<ver>-win64.zip` (exe + DLL Qt) |
| Linux | `./scripts/package.sh` | `.tar.gz` ; AppImage si linuxdeploy est téléchargeable |
| macOS | CI `create-dmg` | `dist/Editerako.dmg` |

`cmake --install <build> --prefix <dir>` exécute aussi `qt_generate_deploy_app_script`. `cpack` (générateurs ZIP / TGZ / DragNDrop) est disponible après un configure Release.
