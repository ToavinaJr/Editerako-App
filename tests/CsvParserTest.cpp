#include "viewers/CsvParser.h"

#include <QtTest>

class CsvParserTest : public QObject
{
    Q_OBJECT

private slots:
    void emptyInput();
    void simpleRows();
    void quotedCommaAndQuotes();
    void crlfAndTrailingNewline();
    void quotedNewline();
};

void CsvParserTest::emptyInput()
{
    QVERIFY(parseCsv(QString()).isEmpty());
}

void CsvParserTest::simpleRows()
{
    const QVector<QStringList> rows = parseCsv(QStringLiteral("a,b,c\n1,2,3"));
    QCOMPARE(rows.size(), 2);
    QCOMPARE(rows.at(0), QStringList({QStringLiteral("a"), QStringLiteral("b"), QStringLiteral("c")}));
    QCOMPARE(rows.at(1), QStringList({QStringLiteral("1"), QStringLiteral("2"), QStringLiteral("3")}));
}

void CsvParserTest::quotedCommaAndQuotes()
{
    const QVector<QStringList> rows = parseCsv(QStringLiteral("\"a,b\",\"says \"\"hi\"\"\""));
    QCOMPARE(rows.size(), 1);
    QCOMPARE(rows.at(0).size(), 2);
    QCOMPARE(rows.at(0).at(0), QStringLiteral("a,b"));
    QCOMPARE(rows.at(0).at(1), QStringLiteral("says \"hi\""));
}

void CsvParserTest::crlfAndTrailingNewline()
{
    const QVector<QStringList> rows = parseCsv(QStringLiteral("a,b\r\nc,d\r\n"));
    QCOMPARE(rows.size(), 2);
    QCOMPARE(rows.at(0), QStringList({QStringLiteral("a"), QStringLiteral("b")}));
    QCOMPARE(rows.at(1), QStringList({QStringLiteral("c"), QStringLiteral("d")}));
}

void CsvParserTest::quotedNewline()
{
    const QVector<QStringList> rows = parseCsv(QStringLiteral("\"a\nb\",c"));
    QCOMPARE(rows.size(), 1);
    QCOMPARE(rows.at(0).size(), 2);
    QCOMPARE(rows.at(0).at(0), QStringLiteral("a\nb"));
    QCOMPARE(rows.at(0).at(1), QStringLiteral("c"));
}

QTEST_GUILESS_MAIN(CsvParserTest)
#include "CsvParserTest.moc"
