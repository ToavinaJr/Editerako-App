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
    QVERIFY(LanguageRegistry::tsLanguage(LanguageId::Html) != nullptr);
    QVERIFY(LanguageRegistry::tsLanguage(LanguageId::PlainText) == nullptr);
    QVERIFY(LanguageRegistry::tsLanguage(LanguageId::Python) == nullptr);
}

void LanguageRegistryTest::highlightQueryPaths()
{
    QCOMPARE(LanguageRegistry::highlightQueryResourcePath(LanguageId::Cpp),
             QStringLiteral(":/editerako/syntax/cpp/highlights.scm"));
    QCOMPARE(LanguageRegistry::highlightQueryResourcePath(LanguageId::C),
             QStringLiteral(":/editerako/syntax/cpp/highlights.scm"));
    QCOMPARE(LanguageRegistry::highlightQueryResourcePath(LanguageId::Html),
             QStringLiteral(":/editerako/syntax/html/highlights.scm"));
    QVERIFY(LanguageRegistry::highlightQueryResourcePath(LanguageId::Python).isEmpty());
}

QTEST_GUILESS_MAIN(LanguageRegistryTest)
#include "LanguageRegistryTest.moc"
