#include "lsp/LspPickerItems.h"
#include "lsp/LspTypes.h"

#include <QDir>
#include <QFileInfo>
#include <QtTest>

class LspPickerItemsTest : public QObject
{
    Q_OBJECT
private slots:
    void emptyLocations();
    void singleLocationUsesFileNameAndLine();
    void symbolFallsBackToPath();
};

void LspPickerItemsTest::emptyLocations()
{
    QCOMPARE(lspLocationRows({}).size(), 0);
    QCOMPARE(lspSymbolRows({}).size(), 0);
}

void LspPickerItemsTest::singleLocationUsesFileNameAndLine()
{
    const QString path = QDir::temp().filePath(QStringLiteral("main.cpp"));
    LspLocation loc;
    loc.uri = lspFileUri(path);
    loc.range.start.line = 11;
    loc.range.start.character = 4;
    const QVector<LspPickerRow> rows = lspLocationRows({loc});
    QCOMPARE(rows.size(), 1);
    QCOMPARE(rows.front().id, QStringLiteral("0"));
    QCOMPARE(rows.front().display, QStringLiteral("main.cpp"));
    QCOMPARE(rows.front().hint, QStringLiteral("main.cpp:12"));
    QCOMPARE(rows.front().line, 11);
    QCOMPARE(rows.front().character, 4);
    QCOMPARE(QFileInfo(rows.front().path).fileName(), QStringLiteral("main.cpp"));
}

void LspPickerItemsTest::symbolFallsBackToPath()
{
    LspSymbol sym;
    sym.name = QStringLiteral("Foo");
    sym.selectionRange.start.line = 2;
    const QVector<LspPickerRow> rows = lspSymbolRows({sym}, QStringLiteral("C:/proj/foo.h"));
    QCOMPARE(rows.size(), 1);
    QCOMPARE(rows.front().display, QStringLiteral("Foo"));
    QCOMPARE(rows.front().path, QStringLiteral("C:/proj/foo.h"));
    QCOMPARE(rows.front().hint, QStringLiteral("foo.h:3"));
    QVERIFY(rows.front().filterText.contains(QStringLiteral("Foo")));
}

QTEST_GUILESS_MAIN(LspPickerItemsTest)
#include "LspPickerItemsTest.moc"
