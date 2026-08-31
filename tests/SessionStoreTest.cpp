#include "core/SessionStore.h"

#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>

class SessionStoreTest : public QObject
{
    Q_OBJECT

private slots:
    void roundTrip();
    void emptySettings();
};

void SessionStoreTest::roundTrip()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    const QString ini = temp.filePath(QStringLiteral("session.ini"));
    QSettings settings(ini, QSettings::IniFormat);

    SessionState original;
    original.workspace = QStringLiteral("/proj");
    original.openFiles = {QStringLiteral("/proj/a.cpp"), QStringLiteral("/proj/b.h")};
    original.activeFile = QStringLiteral("/proj/a.cpp");
    original.geometry = QByteArray("geom");
    original.windowState = QByteArray("state");

    SessionStore store;
    store.save(original, settings);
    settings.sync();

    const SessionState loaded = store.load(settings);
    QCOMPARE(loaded.workspace, original.workspace);
    QCOMPARE(loaded.openFiles, original.openFiles);
    QCOMPARE(loaded.activeFile, original.activeFile);
    QCOMPARE(loaded.geometry, original.geometry);
    QCOMPARE(loaded.windowState, original.windowState);
}

void SessionStoreTest::emptySettings()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    QSettings settings(temp.filePath(QStringLiteral("empty.ini")), QSettings::IniFormat);

    const SessionState loaded = SessionStore().load(settings);
    QVERIFY(loaded.workspace.isEmpty());
    QVERIFY(loaded.openFiles.isEmpty());
    QVERIFY(loaded.activeFile.isEmpty());
}

QTEST_GUILESS_MAIN(SessionStoreTest)
#include "SessionStoreTest.moc"
