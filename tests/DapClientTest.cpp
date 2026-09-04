#include "FakeJsonRpcTransport.h"
#include "debug/DapClient.h"
#include "debug/DapTypes.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QMetaType>
#include <QSignalSpy>
#include <QtTest>

class DapClientTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void initializeHandshake();
    void launchAndSetBreakpoints();
    void stoppedEvent();
    void reverseRequestGetsFailure();
};

void DapClientTest::initTestCase()
{
    qRegisterMetaType<DapStoppedEvent>("DapStoppedEvent");
}

void DapClientTest::initializeHandshake()
{
    FakeJsonRpcTransport transport;
    DapClient client(&transport);
    bool done = false;
    client.initialize(QStringLiteral("gdb"),
                      [&](const QJsonObject &body, const QJsonObject &error) {
                          QVERIFY(error.isEmpty());
                          QVERIFY(body.value(QStringLiteral("supportsConfigurationDoneRequest")).toBool());
                          done = true;
                      });

    QCOMPARE(transport.sent.size(), 1);
    QCOMPARE(transport.sent.front().value(QStringLiteral("type")).toString(),
             QStringLiteral("request"));
    QCOMPARE(transport.sent.front().value(QStringLiteral("command")).toString(),
             QStringLiteral("initialize"));
    const int seq = transport.sent.front().value(QStringLiteral("seq")).toInt();
    QVERIFY(seq > 0);

    transport.inject(QJsonObject{
        {QStringLiteral("seq"), 1},
        {QStringLiteral("type"), QStringLiteral("response")},
        {QStringLiteral("request_seq"), seq},
        {QStringLiteral("success"), true},
        {QStringLiteral("command"), QStringLiteral("initialize")},
        {QStringLiteral("body"),
         QJsonObject{{QStringLiteral("supportsConfigurationDoneRequest"), true}}}});
    QVERIFY(done);
    QVERIFY(client.supportsConfigurationDone());
}

void DapClientTest::launchAndSetBreakpoints()
{
    FakeJsonRpcTransport transport;
    DapClient client(&transport);
    client.launch(QJsonObject{{QStringLiteral("program"), QStringLiteral("/tmp/a.out")}}, {});
    QCOMPARE(transport.sent.back().value(QStringLiteral("command")).toString(),
             QStringLiteral("launch"));

    bool ok = false;
    client.setBreakpoints(QStringLiteral("C:/proj/main.cpp"), {9, 14},
                          [&](const QJsonObject &, const QJsonObject &error) {
                              QVERIFY(error.isEmpty());
                              ok = true;
                          });
    const QJsonObject request = transport.sent.back();
    QCOMPARE(request.value(QStringLiteral("command")).toString(), QStringLiteral("setBreakpoints"));
    const QJsonArray bps =
        request.value(QStringLiteral("arguments")).toObject().value(QStringLiteral("breakpoints")).toArray();
    QCOMPARE(bps.size(), 2);
    QCOMPARE(bps.at(0).toObject().value(QStringLiteral("line")).toInt(), 10);

    transport.inject(QJsonObject{{QStringLiteral("type"), QStringLiteral("response")},
                                 {QStringLiteral("request_seq"), request.value(QStringLiteral("seq"))},
                                 {QStringLiteral("success"), true},
                                 {QStringLiteral("command"), QStringLiteral("setBreakpoints")},
                                 {QStringLiteral("body"), QJsonObject{}}});
    QVERIFY(ok);
}

void DapClientTest::stoppedEvent()
{
    FakeJsonRpcTransport transport;
    DapClient client(&transport);
    QSignalSpy spy(&client, &DapClient::stopped);
    transport.inject(QJsonObject{
        {QStringLiteral("type"), QStringLiteral("event")},
        {QStringLiteral("event"), QStringLiteral("stopped")},
        {QStringLiteral("body"),
         QJsonObject{{QStringLiteral("reason"), QStringLiteral("breakpoint")},
                     {QStringLiteral("threadId"), 3}}}});
    QCOMPARE(spy.size(), 1);
    const auto event = spy.front().front().value<DapStoppedEvent>();
    QCOMPARE(event.reason, QStringLiteral("breakpoint"));
    QCOMPARE(event.threadId, 3);
}

void DapClientTest::reverseRequestGetsFailure()
{
    FakeJsonRpcTransport transport;
    DapClient client(&transport);
    transport.inject(QJsonObject{{QStringLiteral("type"), QStringLiteral("request")},
                                 {QStringLiteral("command"), QStringLiteral("runInTerminal")},
                                 {QStringLiteral("seq"), 9},
                                 {QStringLiteral("arguments"), QJsonObject{}}});
    QVERIFY(!transport.sent.isEmpty());
    const QJsonObject reply = transport.sent.back();
    QCOMPARE(reply.value(QStringLiteral("type")).toString(), QStringLiteral("response"));
    QCOMPARE(reply.value(QStringLiteral("request_seq")).toInt(), 9);
    QCOMPARE(reply.value(QStringLiteral("success")).toBool(), false);
}

QTEST_GUILESS_MAIN(DapClientTest)
#include "DapClientTest.moc"
