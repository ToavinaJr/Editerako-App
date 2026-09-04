#include "debug/DapTypes.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QtTest>

class DapTypesTest : public QObject
{
    Q_OBJECT

private slots:
    void parseStopped();
    void parseFramesAndVariables();
};

void DapTypesTest::parseStopped()
{
    const DapStoppedEvent event = dapStoppedFromJson(
        QJsonObject{{QStringLiteral("reason"), QStringLiteral("step")},
                    {QStringLiteral("threadId"), 8},
                    {QStringLiteral("description"), QStringLiteral("Paused")}});
    QCOMPARE(event.reason, QStringLiteral("step"));
    QCOMPARE(event.threadId, 8);
    QCOMPARE(event.description, QStringLiteral("Paused"));
}

void DapTypesTest::parseFramesAndVariables()
{
    const QJsonObject stack{
        {QStringLiteral("stackFrames"),
         QJsonArray{QJsonObject{{QStringLiteral("id"), 12},
                                {QStringLiteral("name"), QStringLiteral("main")},
                                {QStringLiteral("line"), 42},
                                {QStringLiteral("column"), 1},
                                {QStringLiteral("source"),
                                 QJsonObject{{QStringLiteral("path"), QStringLiteral("C:/a.cpp")}}}}}}};
    const QVector<DapStackFrame> frames = dapStackFramesFromJson(stack);
    QCOMPARE(frames.size(), 1);
    QCOMPARE(frames.front().id, 12);
    QCOMPARE(frames.front().line, 42);
    QCOMPARE(frames.front().sourcePath, QStringLiteral("C:/a.cpp"));

    const QJsonObject vars{
        {QStringLiteral("variables"),
         QJsonArray{QJsonObject{{QStringLiteral("name"), QStringLiteral("x")},
                                {QStringLiteral("value"), QStringLiteral("1")},
                                {QStringLiteral("type"), QStringLiteral("int")},
                                {QStringLiteral("variablesReference"), 0}}}}};
    const QVector<DapVariable> variables = dapVariablesFromJson(vars);
    QCOMPARE(variables.size(), 1);
    QCOMPARE(variables.front().name, QStringLiteral("x"));
    QCOMPARE(variables.front().value, QStringLiteral("1"));
}

QTEST_GUILESS_MAIN(DapTypesTest)
#include "DapTypesTest.moc"
