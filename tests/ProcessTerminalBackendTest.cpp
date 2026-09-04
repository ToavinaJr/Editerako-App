#include "terminal/ProcessTerminalBackend.h"

#include <QDir>
#include <QSignalSpy>
#include <QtTest>

class ProcessTerminalBackendTest : public QObject
{
    Q_OBJECT

private slots:
    void echoCommandProducesOutput();
};

void ProcessTerminalBackendTest::echoCommandProducesOutput()
{
    ProcessTerminalBackend backend;
    QByteArray collected;
    QObject::connect(&backend, &ITerminalBackend::dataReceived, &backend,
                     [&](const QByteArray &data, bool) { collected += data; });
    QSignalSpy finished(&backend, &ITerminalBackend::finished);

#ifdef Q_OS_WIN
    backend.start(QStringLiteral("cmd"),
                  {QStringLiteral("/c"), QStringLiteral("echo hello-proc")}, QDir::tempPath(), 80,
                  24);
#else
    backend.start(QStringLiteral("echo"), {QStringLiteral("hello-proc")}, QDir::tempPath(), 80, 24);
#endif

    QVERIFY(finished.wait(10000));
    QVERIFY(QString::fromLocal8Bit(collected).contains(QStringLiteral("hello-proc")));
    QCOMPARE(finished.front().at(0).toInt(), 0);
}

QTEST_GUILESS_MAIN(ProcessTerminalBackendTest)
#include "ProcessTerminalBackendTest.moc"
