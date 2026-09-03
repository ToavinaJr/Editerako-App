#include "core/SessionController.h"

#include <QDir>
#include <QFile>
#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>

class SessionControllerTest : public QObject
{
    Q_OBJECT

private slots:
    void workspaceIsRestorable();
    void existingFilesKeepsOrder();
    void saveSkippedWhileRestoring();
    void saveAfterRestoreGuard();
};

void SessionControllerTest::workspaceIsRestorable()
{
    SessionState empty;
    QVERIFY(!SessionController::workspaceIsRestorable(empty));

    SessionState missing;
    missing.workspace = QStringLiteral("/this/path/should/not/exist/editerako-session");
    QVERIFY(!SessionController::workspaceIsRestorable(missing));

    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    SessionState ok;
    ok.workspace = temp.path();
    QVERIFY(SessionController::workspaceIsRestorable(ok));
}

void SessionControllerTest::existingFilesKeepsOrder()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    const QString a = temp.filePath(QStringLiteral("a.txt"));
    const QString b = temp.filePath(QStringLiteral("b.txt"));
    QVERIFY(QFile(a).open(QIODevice::WriteOnly));
    QVERIFY(QFile(b).open(QIODevice::WriteOnly));

    const QStringList filtered = SessionController::existingFiles(
        {a, QStringLiteral("/no/such/file"), b});
    QCOMPARE(filtered, (QStringList{a, b}));
}

void SessionControllerTest::saveSkippedWhileRestoring()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    QSettings settings(temp.filePath(QStringLiteral("session.ini")), QSettings::IniFormat);

    SessionController controller;
    SessionState original;
    original.workspace = QStringLiteral("/before");
    controller.save(original, settings);

    {
        SessionController::RestoreGuard guard(controller);
        QVERIFY(controller.isRestoring());
        SessionState during;
        during.workspace = QStringLiteral("/during-restore");
        controller.save(during, settings);
    }

    const SessionState loaded = controller.load(settings);
    QCOMPARE(loaded.workspace, QStringLiteral("/before"));
}

void SessionControllerTest::saveAfterRestoreGuard()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    QSettings settings(temp.filePath(QStringLiteral("session.ini")), QSettings::IniFormat);

    SessionController controller;
    {
        SessionController::RestoreGuard guard(controller);
    }
    QVERIFY(!controller.isRestoring());

    SessionState after;
    after.workspace = QStringLiteral("/after");
    after.openFiles = {QStringLiteral("/after/a.cpp")};
    controller.save(after, settings);

    const SessionState loaded = controller.load(settings);
    QCOMPARE(loaded.workspace, QStringLiteral("/after"));
    QCOMPARE(loaded.openFiles, after.openFiles);
}

QTEST_GUILESS_MAIN(SessionControllerTest)
#include "SessionControllerTest.moc"
