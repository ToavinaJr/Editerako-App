#include "editor/features/AutoClosingPairs.h"
#include "editor/features/BracketMatcher.h"

#include <QtTest>

class BracketMatcherTest : public QObject
{
    Q_OBJECT

private slots:
    void matchesForwardAndBack();
    void autoCloseRules();
};

void BracketMatcherTest::matchesForwardAndBack()
{
    const QString text = QStringLiteral("a(b[c])");
    const BracketMatch paren = findBracketMatch(text, 1);
    QVERIFY(paren.isValid());
    QCOMPARE(paren.open, 1);
    QCOMPARE(paren.close, 6);

    const BracketMatch fromClose = findBracketMatch(text, 7);
    QVERIFY(fromClose.isValid());
    QCOMPARE(fromClose.open, 1);
    QCOMPARE(fromClose.close, 6);

    QVERIFY(!findBracketMatch(QStringLiteral("abc"), 1).isValid());
}

void BracketMatcherTest::autoCloseRules()
{
    QCOMPARE(closingPairFor(QLatin1Char('(')), QLatin1Char(')'));
    QVERIFY(shouldInsertClosingPair(QStringLiteral("ab"), 2, QLatin1Char('(')));
    QVERIFY(!shouldInsertClosingPair(QStringLiteral("it"), 2, QLatin1Char('\'')));
    QVERIFY(shouldSkipClosingPair(QStringLiteral("()"), 1, QLatin1Char(')')));
}

QTEST_GUILESS_MAIN(BracketMatcherTest)
#include "BracketMatcherTest.moc"
