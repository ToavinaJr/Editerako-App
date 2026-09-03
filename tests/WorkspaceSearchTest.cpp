#include "project/WorkspaceSearch.h"

#include <QtTest>

class WorkspaceSearchTest : public QObject
{
    Q_OBJECT

private slots:
    void findsLiteralCaseAndWord();
    void regexAndReplace();
    void invalidRegex();
};

void WorkspaceSearchTest::findsLiteralCaseAndWord()
{
    const QString text = QStringLiteral("Foo foo food\nbar\n");
    SearchOptions options;
    options.query = QStringLiteral("foo");

    QList<SearchHit> hits = findInText(text, QStringLiteral("a.txt"), compileSearch(options));
    QCOMPARE(hits.size(), 3);

    options.caseSensitive = true;
    hits = findInText(text, QStringLiteral("a.txt"), compileSearch(options));
    QCOMPARE(hits.size(), 2);

    options.caseSensitive = false;
    options.wholeWord = true;
    hits = findInText(text, QStringLiteral("a.txt"), compileSearch(options));
    QCOMPARE(hits.size(), 2);
    QCOMPARE(hits.at(0).line, 1);
}

void WorkspaceSearchTest::regexAndReplace()
{
    SearchOptions options;
    options.query = QStringLiteral("f.o");
    options.regex = true;
    const QString text = QStringLiteral("foo fao bar");
    const CompiledSearch compiled = compileSearch(options);
    QVERIFY(compiled.isValid());
    QCOMPARE(findInText(text, QStringLiteral("a.txt"), compiled).size(), 2);

    int count = 0;
    QCOMPARE(replaceInText(text, compiled, QStringLiteral("X"), &count),
             QStringLiteral("X X bar"));
    QCOMPARE(count, 2);
}

void WorkspaceSearchTest::invalidRegex()
{
    SearchOptions options;
    options.query = QStringLiteral("(");
    options.regex = true;
    QVERIFY(!compileSearch(options).isValid());
}

QTEST_GUILESS_MAIN(WorkspaceSearchTest)
#include "WorkspaceSearchTest.moc"
