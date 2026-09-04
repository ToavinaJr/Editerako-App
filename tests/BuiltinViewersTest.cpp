#include "viewers/CsvViewer.h"
#include "viewers/MarkdownViewer.h"
#include "viewers/SvgViewer.h"

#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

class BuiltinViewersTest : public QObject
{
    Q_OBJECT

private slots:
    void markdownLoads();
    void svgLoadsValidAndRejectsGarbage();
    void csvLoadsTable();
};

void BuiltinViewersTest::markdownLoads()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("note.md"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    file.write("# Title\n\nHello **world**.\n");
    file.close();

    MarkdownViewer viewer;
    QVERIFY(viewer.load(path));
    QCOMPARE(viewer.filePath(), path);
    QVERIFY(viewer.windowTitle().contains(QStringLiteral("Preview")));
}

void BuiltinViewersTest::svgLoadsValidAndRejectsGarbage()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString good = dir.filePath(QStringLiteral("ok.svg"));
    const QString bad = dir.filePath(QStringLiteral("bad.svg"));

    QFile goodFile(good);
    QVERIFY(goodFile.open(QIODevice::WriteOnly | QIODevice::Truncate));
    goodFile.write(
        "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"8\" height=\"8\">"
        "<rect width=\"8\" height=\"8\" fill=\"#f00\"/></svg>");
    goodFile.close();

    QFile badFile(bad);
    QVERIFY(badFile.open(QIODevice::WriteOnly | QIODevice::Truncate));
    badFile.write("not an svg");
    badFile.close();

    SvgViewer viewer;
    QVERIFY(viewer.load(good));
    QCOMPARE(viewer.filePath(), good);
    QVERIFY(!viewer.load(bad));
}

void BuiltinViewersTest::csvLoadsTable()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("data.csv"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    file.write("name,count\nalpha,1\nbeta,2\n");
    file.close();

    CsvViewer viewer;
    QVERIFY(viewer.load(path));
    QCOMPARE(viewer.filePath(), path);
}

QTEST_MAIN(BuiltinViewersTest)
#include "BuiltinViewersTest.moc"
