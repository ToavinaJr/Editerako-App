#include "ai/ContextBuilder.h"

#include <QtTest>

class ContextBuilderTest : public QObject
{
    Q_OBJECT

private slots:
    void userMessageOnly();
    void includesActiveFile();
    void truncatesLongFile();
    void historyWindowAndDuplicateSkip();
};

void ContextBuilderTest::userMessageOnly()
{
    ContextBuilder builder;
    QCOMPARE(builder.buildPrompt(QStringLiteral("hello"), {}), QStringLiteral("hello"));
}

void ContextBuilderTest::includesActiveFile()
{
    ContextBuilder builder;
    builder.setActiveFile(QStringLiteral("main.cpp"), QStringLiteral("int main() {}"));
    const QString prompt = builder.buildPrompt(QStringLiteral("explain"), {});
    QVERIFY(prompt.contains(QStringLiteral("Current file: main.cpp")));
    QVERIFY(prompt.contains(QStringLiteral("int main() {}")));
    QVERIFY(prompt.endsWith(QStringLiteral("explain")));
}

void ContextBuilderTest::truncatesLongFile()
{
    ContextBuilder builder;
    builder.setActiveFile(QStringLiteral("big.txt"), QString(9000, QLatin1Char('a')));
    const QString prompt = builder.buildPrompt(QStringLiteral("q"), {});
    QVERIFY(prompt.contains(QStringLiteral("... [truncated]")));
    QVERIFY(!prompt.contains(QString(9000, QLatin1Char('a'))));
}

void ContextBuilderTest::historyWindowAndDuplicateSkip()
{
    ContextBuilder builder;
    QList<ChatMessage> history;
    for (int i = 0; i < 10; ++i) {
        history.append({QStringLiteral("user"), QStringLiteral("m%1").arg(i)});
    }
    history.append({QStringLiteral("user"), QStringLiteral("latest")});

    const QString prompt = builder.buildPrompt(QStringLiteral("latest"), history);
    QVERIFY(!prompt.contains(QStringLiteral("m0")));
    QVERIFY(!prompt.contains(QStringLiteral("m1")));
    QVERIFY(!prompt.contains(QStringLiteral("m2")));
    QVERIFY(prompt.contains(QStringLiteral("m3")));
    QCOMPARE(prompt.count(QStringLiteral("latest")), 1);
}

QTEST_GUILESS_MAIN(ContextBuilderTest)
#include "ContextBuilderTest.moc"
