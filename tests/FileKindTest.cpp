#include "syntax/LanguageRegistry.h"
#include "viewers/FileKind.h"

#include <QtTest>

class FileKindTest : public QObject
{
    Q_OBJECT

private slots:
    void emptyIsUnsupported();
    void textByExtension();
    void pdfAndImage();
    void extraLanguageIsText();
};

void FileKindTest::emptyIsUnsupported()
{
    QCOMPARE(fileKindForPath({}), FileKind::Unsupported);
}

void FileKindTest::textByExtension()
{
    QCOMPARE(fileKindForPath(QStringLiteral("main.cpp")), FileKind::Text);
    QCOMPARE(fileKindForPath(QStringLiteral("notes.txt")), FileKind::Text);
    QCOMPARE(fileKindForPath(QStringLiteral("app.tsx")), FileKind::Text);
    QCOMPARE(fileKindForPath(QStringLiteral("data.json")), FileKind::Text);
}

void FileKindTest::pdfAndImage()
{
    QCOMPARE(fileKindForPath(QStringLiteral("doc.pdf")), FileKind::Pdf);
    QCOMPARE(fileKindForPath(QStringLiteral("shot.png")), FileKind::Image);
    QCOMPARE(fileKindForPath(QStringLiteral("photo.jpg")), FileKind::Image);
}

void FileKindTest::extraLanguageIsText()
{
    LanguageRegistry::ExtraLanguage toml;
    toml.displayName = QStringLiteral("TOML");
    toml.extensions = QStringList{QStringLiteral("toml")};
    LanguageRegistry::setExtraLanguages({toml});
    QCOMPARE(fileKindForPath(QStringLiteral("Cargo.toml")), FileKind::Text);
    LanguageRegistry::setExtraLanguages({});
}

QTEST_GUILESS_MAIN(FileKindTest)
#include "FileKindTest.moc"
