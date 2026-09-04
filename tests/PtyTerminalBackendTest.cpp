#include "terminal/AnsiSgr.h"
#include "terminal/PtyTerminalBackend.h"

#include <QDir>
#include <QSignalSpy>
#include <QtTest>

class PtyTerminalBackendTest : public QObject
{
    Q_OBJECT

private slots:
    void reportsAvailability();
    void echoCommandWhenAvailable();
};

void PtyTerminalBackendTest::reportsAvailability()
{
    QVERIFY(PtyTerminalBackend::isAvailable());
    PtyTerminalBackend backend;
    QVERIFY(!backend.isRunning());
    QVERIFY(backend.isPty());
    backend.stop();
}

void PtyTerminalBackendTest::echoCommandWhenAvailable()
{
    if (!PtyTerminalBackend::isAvailable()) {
        QSKIP("PTY / ConPTY is not available on this host");
    }
#ifdef Q_OS_WIN
    QSKIP("ConPTY one-shot from a console test host attaches to the parent console; "
          "the GUI app (WIN32) is the supported path");
#endif

    PtyTerminalBackend backend;
    QByteArray collected;
    QObject::connect(&backend, &ITerminalBackend::dataReceived, &backend,
                     [&](const QByteArray &data, bool) { collected += data; });
    QSignalSpy finished(&backend, &ITerminalBackend::finished);

    backend.start(QStringLiteral("/bin/sh"), {QStringLiteral("-c"), QStringLiteral("echo hello-pty")},
                  QDir::tempPath(), 80, 24);

    QVERIFY2(backend.isRunning() || finished.size() > 0, "PTY process did not start");
    if (finished.isEmpty()) {
        QVERIFY(finished.wait(15000));
    }
    const QString text = stripAnsi(QString::fromLocal8Bit(collected));
    QVERIFY2(text.contains(QStringLiteral("hello-pty")), qPrintable(text));
}

QTEST_GUILESS_MAIN(PtyTerminalBackendTest)
#include "PtyTerminalBackendTest.moc"
