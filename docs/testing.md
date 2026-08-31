# Tests

Suite **Qt Test** + **ctest**. On teste la logique isolée, pas `MainWindow` ni les widgets lourds (terminal, PDF).

```powershell
.\scripts\test.ps1
```

```bash
./scripts/test.sh
```

Équivalent : `ctest --preset debug --output-on-failure`.

## Cibles actuelles

| Cible | Fichiers sous test |
|---|---|
| `test_LanguageRegistry` | extensions C++ / HTML, pointeurs Tree-sitter |
| `test_Workspace` | racine, `containsPath`, exclusions, création fichier/dossier |
| `test_DropPaths` | extraction de chemins depuis `QMimeData` |
| `test_SessionStore` | round-trip via `QSettings` Ini temporaire |
| `test_ContextBuilder` | prompt, troncature, fenêtre d’historique |
| `test_FileKind` | texte / PDF / image / vide |
| `test_CommandRegistry` | enregistrement, doublons, `setEnabled` (`QT_QPA_PLATFORM=offscreen`) |

Les binaires sont dans `build/<preset>/tests/`.

## Ajouter un test

1. Créer `tests/FooTest.cpp` :

```cpp
#include "module/Foo.h"
#include <QtTest>

class FooTest : public QObject
{
    Q_OBJECT
private slots:
    void example();
};

void FooTest::example()
{
    QCOMPARE(1 + 1, 2);
}

QTEST_GUILESS_MAIN(FooTest)   // QTEST_MAIN si QWidget / QApplication
#include "FooTest.moc"
```

2. Dans `tests/CMakeLists.txt` :

```cmake
editerako_add_test(test_Foo
    SOURCES FooTest.cpp
            "${PROJECT_SOURCE_DIR}/src/module/Foo.cpp"
    LIBS Qt6::Core
)
```

Compiler les `.cpp` nécessaires du module (et `Logging.cpp` si le code utilise `qCInfo`). Ne pas lier `Editerako.exe`.

3. Si le test crée un `QWidget`, lier `Qt6::Widgets`, utiliser `QTEST_MAIN`, et éventuellement :

```cmake
set_tests_properties(test_Foo PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
```

4. `.\scripts\test.ps1` — tout doit rester vert.

Isoler le disque et `QSettings` avec `QTemporaryDir`. Ne pas écrire dans le profil `Editerako` de l’utilisateur.
