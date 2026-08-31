#include "syntax/LanguageRegistry.h"

#include <QtTest>

class LanguageRegistryTest : public QObject
{
    Q_OBJECT

private slots:
    void emptyPathIsPlainText();
    void cppExtensions();
    void htmlExtensions();
    void unknownExtensionIsPlainText();
    void displayNames();
    void treeSitterLanguages();
};

void LanguageRegistryTest::emptyPathIsPlainText()
{
    QCOMPARE(LanguageRegistry::idForPath({}), LanguageId::PlainText);
}

void LanguageRegistryTest::cppExtensions()
{
    QCOMPARE(LanguageRegistry::idForPath(QStringLiteral("main.cpp")), LanguageId::Cpp);
    QCOMPARE(LanguageRegistry::idForPath(QStringLiteral("foo.HPP")), LanguageId::Cpp);
    QCOMPARE(LanguageRegistry::idForPath(QStringLiteral("/tmp/a.cc")), LanguageId::Cpp);
    QCOMPARE(LanguageRegistry::idForExtension(QStringLiteral("h")), LanguageId::Cpp);
}

void LanguageRegistryTest::htmlExtensions()
{
    QCOMPARE(LanguageRegistry::idForPath(QStringLiteral("index.html")), LanguageId::Html);
    QCOMPARE(LanguageRegistry::idForExtension(QStringLiteral("HTM")), LanguageId::Html);
}

void LanguageRegistryTest::unknownExtensionIsPlainText()
{
    QCOMPARE(LanguageRegistry::idForPath(QStringLiteral("notes.txt")), LanguageId::PlainText);
    QCOMPARE(LanguageRegistry::idForPath(QStringLiteral("app.rs")), LanguageId::PlainText);
}

void LanguageRegistryTest::displayNames()
{
    QCOMPARE(LanguageRegistry::displayName(LanguageId::Cpp), QStringLiteral("C++"));
    QCOMPARE(LanguageRegistry::displayName(LanguageId::Html), QStringLiteral("HTML"));
    QCOMPARE(LanguageRegistry::displayName(LanguageId::PlainText), QStringLiteral("Plain Text"));
}

void LanguageRegistryTest::treeSitterLanguages()
{
    QVERIFY(LanguageRegistry::tsLanguage(LanguageId::Cpp) != nullptr);
    QVERIFY(LanguageRegistry::tsLanguage(LanguageId::Html) != nullptr);
    QVERIFY(LanguageRegistry::tsLanguage(LanguageId::PlainText) == nullptr);
}

QTEST_GUILESS_MAIN(LanguageRegistryTest)
#include "LanguageRegistryTest.moc"
