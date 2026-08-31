#include "terminal/CommandHistory.h"

#include <QtTest>

class CommandHistoryTest : public QObject
{
    Q_OBJECT

private slots:
    void ignoresEmpty();
    void skipsDuplicateConsecutive();
    void upDownAndPastEnd();
};

void CommandHistoryTest::ignoresEmpty()
{
    CommandHistory history;
    history.add(QString());
    history.add(QStringLiteral("   "));
    QVERIFY(history.isEmpty());
}

void CommandHistoryTest::skipsDuplicateConsecutive()
{
    CommandHistory history;
    history.add(QStringLiteral("ls"));
    history.add(QStringLiteral("ls"));
    history.add(QStringLiteral("pwd"));
    QCOMPARE(history.size(), 2);
    QCOMPARE(history.entries().at(0), QStringLiteral("ls"));
    QCOMPARE(history.entries().at(1), QStringLiteral("pwd"));
}

void CommandHistoryTest::upDownAndPastEnd()
{
    CommandHistory history;
    QVERIFY(!history.navigate(-1).applied);

    history.add(QStringLiteral("one"));
    history.add(QStringLiteral("two"));

    auto up = history.navigate(-1);
    QVERIFY(up.applied);
    QVERIFY(!up.clearLine);
    QCOMPARE(up.command, QStringLiteral("two"));

    up = history.navigate(-1);
    QCOMPARE(up.command, QStringLiteral("one"));

    up = history.navigate(-1);
    QCOMPARE(up.command, QStringLiteral("one"));

    auto down = history.navigate(1);
    QCOMPARE(down.command, QStringLiteral("two"));

    down = history.navigate(1);
    QVERIFY(down.applied);
    QVERIFY(down.clearLine);
    QVERIFY(down.command.isEmpty());
}

QTEST_GUILESS_MAIN(CommandHistoryTest)
#include "CommandHistoryTest.moc"
