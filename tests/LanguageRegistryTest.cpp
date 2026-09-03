#include "syntax/LanguageRegistry.h"

#include <QtTest>

class LanguageRegistryTest : public QObject
{
    Q_OBJECT

private slots:
    void emptyPathIsPlainText();
    void cAndCppExtensions();
    void htmlExtensions();
    void futureLanguageExtensions();
    void cmakeListsByFileName();
    void unknownExtensionIsPlainText();
    void displayNames();
    void treeSitterLanguages();
    void highlightQueryPaths();
    void commentTokens();
    void definitionTableCoversEveryId();
};

void LanguageRegistryTest::emptyPathIsPlainText()
{
    QCOMPARE(LanguageRegistry::idForPath({}), LanguageId::PlainText);
}

void LanguageRegistryTest::cAndCppExtensions()
{
    QCOMPARE(LanguageRegistry::idForPath(QStringLiteral("main.cpp")), LanguageId::Cpp);
    QCOMPARE(LanguageRegistry::idForPath(QStringLiteral("foo.HPP")), LanguageId::Cpp);
    QCOMPARE(LanguageRegistry::idForPath(QStringLiteral("/tmp/a.cc")), LanguageId::Cpp);
    QCOMPARE(LanguageRegistry::idForExtension(QStringLiteral("h")), LanguageId::Cpp);
    QCOMPARE(LanguageRegistry::idForPath(QStringLiteral("util.c")), LanguageId::C);
}

void LanguageRegistryTest::htmlExtensions()
{
    QCOMPARE(LanguageRegistry::idForPath(QStringLiteral("index.html")), LanguageId::Html);
    QCOMPARE(LanguageRegistry::idForExtension(QStringLiteral("HTM")), LanguageId::Html);
}

void LanguageRegistryTest::futureLanguageExtensions()
{
    QCOMPARE(LanguageRegistry::idForPath(QStringLiteral("app.py")), LanguageId::Python);
    QCOMPARE(LanguageRegistry::idForPath(QStringLiteral("main.ts")), LanguageId::TypeScript);
    QCOMPARE(LanguageRegistry::idForPath(QStringLiteral("App.tsx")), LanguageId::Tsx);
    QCOMPARE(LanguageRegistry::idForPath(QStringLiteral("index.js")), LanguageId::JavaScript);
    QCOMPARE(LanguageRegistry::idForPath(QStringLiteral("style.css")), LanguageId::Css);
    QCOMPARE(LanguageRegistry::idForPath(QStringLiteral("data.json")), LanguageId::Json);
    QCOMPARE(LanguageRegistry::idForPath(QStringLiteral("README.md")), LanguageId::Markdown);
    QCOMPARE(LanguageRegistry::idForPath(QStringLiteral("run.sh")), LanguageId::Shell);
    QCOMPARE(LanguageRegistry::idForPath(QStringLiteral("schema.sql")), LanguageId::Sql);
    QCOMPARE(LanguageRegistry::idForPath(QStringLiteral("ci.yml")), LanguageId::Yaml);
}

void LanguageRegistryTest::cmakeListsByFileName()
{
    QCOMPARE(LanguageRegistry::idForPath(QStringLiteral("CMakeLists.txt")), LanguageId::CMake);
    QCOMPARE(LanguageRegistry::idForPath(QStringLiteral("Foo.cmake")), LanguageId::CMake);
}

void LanguageRegistryTest::unknownExtensionIsPlainText()
{
    QCOMPARE(LanguageRegistry::idForPath(QStringLiteral("notes.txt")), LanguageId::PlainText);
    QCOMPARE(LanguageRegistry::idForPath(QStringLiteral("app.rs")), LanguageId::PlainText);
}

void LanguageRegistryTest::displayNames()
{
    QCOMPARE(LanguageRegistry::displayName(LanguageId::C), QStringLiteral("C"));
    QCOMPARE(LanguageRegistry::displayName(LanguageId::Cpp), QStringLiteral("C++"));
    QCOMPARE(LanguageRegistry::displayName(LanguageId::Html), QStringLiteral("HTML"));
    QCOMPARE(LanguageRegistry::displayName(LanguageId::Python), QStringLiteral("Python"));
    QCOMPARE(LanguageRegistry::displayName(LanguageId::PlainText), QStringLiteral("Plain Text"));
}

void LanguageRegistryTest::treeSitterLanguages()
{
    QVERIFY(LanguageRegistry::tsLanguage(LanguageId::Cpp) != nullptr);
    QVERIFY(LanguageRegistry::tsLanguage(LanguageId::C) != nullptr);
    QVERIFY(LanguageRegistry::tsLanguage(LanguageId::C) != LanguageRegistry::tsLanguage(LanguageId::Cpp));
    QVERIFY(LanguageRegistry::tsLanguage(LanguageId::Html) != nullptr);
    QVERIFY(LanguageRegistry::tsLanguage(LanguageId::Python) != nullptr);
    QVERIFY(LanguageRegistry::tsLanguage(LanguageId::JavaScript) != nullptr);
    QVERIFY(LanguageRegistry::tsLanguage(LanguageId::TypeScript) != nullptr);
    QVERIFY(LanguageRegistry::tsLanguage(LanguageId::Tsx) != nullptr);
    QVERIFY(LanguageRegistry::tsLanguage(LanguageId::Json) != nullptr);
    QVERIFY(LanguageRegistry::tsLanguage(LanguageId::Css) != nullptr);
    QVERIFY(LanguageRegistry::tsLanguage(LanguageId::Markdown) != nullptr);
    QVERIFY(LanguageRegistry::tsLanguage(LanguageId::Shell) != nullptr);
    QVERIFY(LanguageRegistry::tsLanguage(LanguageId::Sql) != nullptr);
    QVERIFY(LanguageRegistry::tsLanguage(LanguageId::Yaml) != nullptr);
    QVERIFY(LanguageRegistry::tsLanguage(LanguageId::CMake) != nullptr);
    QVERIFY(LanguageRegistry::tsLanguage(LanguageId::PlainText) == nullptr);
}

void LanguageRegistryTest::highlightQueryPaths()
{
    QCOMPARE(LanguageRegistry::highlightQueryResourcePath(LanguageId::Cpp),
             QStringLiteral(":/editerako/syntax/cpp/highlights.scm"));
    QCOMPARE(LanguageRegistry::highlightQueryResourcePath(LanguageId::C),
             QStringLiteral(":/editerako/syntax/c/highlights.scm"));
    QCOMPARE(LanguageRegistry::highlightQueryResourcePath(LanguageId::Html),
             QStringLiteral(":/editerako/syntax/html/highlights.scm"));
    QCOMPARE(LanguageRegistry::highlightQueryResourcePath(LanguageId::Python),
             QStringLiteral(":/editerako/syntax/python/highlights.scm"));
    QVERIFY(LanguageRegistry::highlightQueryResourcePath(LanguageId::PlainText).isEmpty());
}

void LanguageRegistryTest::commentTokens()
{
    QCOMPARE(LanguageRegistry::commentTokens(LanguageId::Cpp).line, QStringLiteral("//"));
    QCOMPARE(LanguageRegistry::commentTokens(LanguageId::Python).line, QStringLiteral("#"));
    QCOMPARE(LanguageRegistry::commentTokens(LanguageId::Html).blockOpen, QStringLiteral("<!--"));
    QVERIFY(LanguageRegistry::commentTokens(LanguageId::PlainText).line.isEmpty());
}

void LanguageRegistryTest::definitionTableCoversEveryId()
{
    const auto &defs = LanguageRegistry::all();
    QVERIFY(defs.size() >= 15);
    for (const LanguageDefinition &def : defs) {
        QCOMPARE(LanguageRegistry::definition(def.id).displayName, def.displayName);
        if (def.id == LanguageId::PlainText) {
            QVERIFY(def.treeSitterLanguage == nullptr);
            QVERIFY(def.highlightQueryResourcePath.isEmpty());
            continue;
        }
        QVERIFY2(def.treeSitterLanguage != nullptr, qPrintable(def.displayName));
        QVERIFY2(!def.highlightQueryResourcePath.isEmpty(), qPrintable(def.displayName));
        QVERIFY2(LanguageRegistry::tsLanguage(def.id) != nullptr, qPrintable(def.displayName));
    }
}

QTEST_GUILESS_MAIN(LanguageRegistryTest)
#include "LanguageRegistryTest.moc"
