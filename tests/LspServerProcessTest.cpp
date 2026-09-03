#include "lsp/LspServerManager.h"
#include "lsp/LspServerProcess.h"

#include <QDir>
#include <QFileInfo>
#include <QSignalSpy>
#include <QtTest>

class LspServerProcessTest : public QObject
{
    Q_OBJECT

private slots:
    void emptyCommandFails();
    void missingBinaryEmitsFailed();
    void unknownSpecFails();
};

void LspServerProcessTest::emptyCommandFails()
{
    LspServerProcess process;
    QSignalSpy spy(&process, &LspServerProcess::failed);
    QVERIFY(!process.start(QString(), {}, {}));
    QCOMPARE(spy.size(), 1);
    QVERIFY(!process.isRunning());
}

void LspServerProcessTest::missingBinaryEmitsFailed()
{
    LspServerProcess process;
    QSignalSpy spy(&process, &LspServerProcess::failed);
    const QString missing =
        QDir::temp().absoluteFilePath(QStringLiteral("editerako-no-such-lsp-server.exe"));
    QVERIFY(!QFileInfo::exists(missing));
    QVERIFY(!process.start(missing, {}, {}));
    QCOMPARE(spy.size(), 1);
    QVERIFY(!process.isRunning());
}

void LspServerProcessTest::unknownSpecFails()
{
    LspServerManager manager;
    QSignalSpy spy(&manager, &LspServerManager::serverFailed);
    QVERIFY(!manager.startSpec(QStringLiteral("missing"), QStringLiteral("file:///tmp")));
    QCOMPARE(spy.size(), 1);
}

QTEST_GUILESS_MAIN(LspServerProcessTest)
#include "LspServerProcessTest.moc"
