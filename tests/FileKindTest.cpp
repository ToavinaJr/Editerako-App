#include "viewers/FileKind.h"

#include <QtTest>

class FileKindTest : public QObject
{
    Q_OBJECT

private slots:
    void emptyIsUnsupported();
    void textByExtension();
    void pdfAndImage();
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

QTEST_GUILESS_MAIN(FileKindTest)
#include "FileKindTest.moc"
