#include "core/FuzzyMatcher.h"

#include <QtTest>

class FuzzyMatcherTest : public QObject
{
    Q_OBJECT

private slots:
    void emptyQueryMatches();
    void subsequenceAndMiss();
    void caseInsensitive();
    void prefixOutranksLaterMatch();
    void rankLimitsResults();
    void parsesFileAndLine();
};

void FuzzyMatcherTest::emptyQueryMatches()
{
    QCOMPARE(fuzzyScore(QStringLiteral("src/main.cpp"), {}), 0);
    const QList<FuzzyMatch> matches = fuzzyRank({QStringLiteral("a"), QStringLiteral("b")}, {});
    QCOMPARE(matches.size(), 2);
}

void FuzzyMatcherTest::subsequenceAndMiss()
{
    QVERIFY(fuzzyScore(QStringLiteral("fuzzy"), QStringLiteral("fzy")) >= 0);
    QCOMPARE(fuzzyScore(QStringLiteral("abc"), QStringLiteral("xyz")), -1);
}

void FuzzyMatcherTest::caseInsensitive()
{
    QVERIFY(fuzzyScore(QStringLiteral("MainWindow.cpp"), QStringLiteral("main")) >= 0);
}

void FuzzyMatcherTest::prefixOutranksLaterMatch()
{
    const QStringList files{
        QStringLiteral("vendor/editor.cpp"),
        QStringLiteral("src/editor.cpp"),
    };
    const QList<FuzzyMatch> matches = fuzzyRank(files, QStringLiteral("editor.cpp"));
    QCOMPARE(matches.size(), 2);
    QCOMPARE(files.at(matches.first().index), QStringLiteral("src/editor.cpp"));
}

void FuzzyMatcherTest::rankLimitsResults()
{
    const QStringList files{QStringLiteral("a.txt"), QStringLiteral("b.txt"), QStringLiteral("c.txt")};
    QCOMPARE(fuzzyRank(files, QStringLiteral("txt"), 2).size(), 2);
}

void FuzzyMatcherTest::parsesFileAndLine()
{
    const FileLineQuery withLine = parseFileLineQuery(QStringLiteral("src/main.cpp:12"));
    QCOMPARE(withLine.pattern, QStringLiteral("src/main.cpp"));
    QCOMPARE(withLine.line, 12);

    const FileLineQuery plain = parseFileLineQuery(QStringLiteral("src/main.cpp"));
    QCOMPARE(plain.pattern, QStringLiteral("src/main.cpp"));
    QCOMPARE(plain.line, 0);

    const FileLineQuery invalid = parseFileLineQuery(QStringLiteral("src/main.cpp:abc"));
    QCOMPARE(invalid.pattern, QStringLiteral("src/main.cpp:abc"));
    QCOMPARE(invalid.line, 0);

    const FileLineQuery windows = parseFileLineQuery(QStringLiteral("C:/proj/main.cpp:8"));
    QCOMPARE(windows.pattern, QStringLiteral("C:/proj/main.cpp"));
    QCOMPARE(windows.line, 8);
}

QTEST_GUILESS_MAIN(FuzzyMatcherTest)
#include "FuzzyMatcherTest.moc"
