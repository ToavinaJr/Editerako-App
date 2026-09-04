#include "core/RecentWorkspaces.h"

#include <QDir>
#include <QList>
#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>

class RecentWorkspacesTest : public QObject
{
    Q_OBJECT

private slots:
    void rememberMovesToFrontAndDedupes();
    void capsAtMaxEntries();
    void pruneDropsMissing();
    void removeAndClear();
    void ignoresNonexistent();
};

void RecentWorkspacesTest::rememberMovesToFrontAndDedupes()
{
    QTemporaryDir first;
    QTemporaryDir second;
    QVERIFY(first.isValid());
    QVERIFY(second.isValid());
    QTemporaryDir storeDir;
    QSettings settings(storeDir.filePath(QStringLiteral("recent.ini")), QSettings::IniFormat);
    RecentWorkspaces recents(settings);

    recents.remember(first.path());
    recents.remember(second.path());
    recents.remember(first.path());

    const QStringList entries = recents.entries();
    QCOMPARE(entries.size(), 2);
    QVERIFY(RecentWorkspaces::samePath(entries.at(0), first.path()));
    QVERIFY(RecentWorkspaces::samePath(entries.at(1), second.path()));
}

void RecentWorkspacesTest::capsAtMaxEntries()
{
    QTemporaryDir storeDir;
    QSettings settings(storeDir.filePath(QStringLiteral("recent.ini")), QSettings::IniFormat);
    RecentWorkspaces recents(settings);

    QList<QTemporaryDir *> dirs;
    dirs.reserve(RecentWorkspaces::MaxEntries + 3);
    for (int i = 0; i < RecentWorkspaces::MaxEntries + 3; ++i) {
        auto *dir = new QTemporaryDir;
        QVERIFY(dir->isValid());
        dirs.append(dir);
        recents.remember(dir->path());
    }

    QCOMPARE(recents.entries().size(), RecentWorkspaces::MaxEntries);
    QVERIFY(RecentWorkspaces::samePath(recents.entries().first(), dirs.last()->path()));

    qDeleteAll(dirs);
}

void RecentWorkspacesTest::pruneDropsMissing()
{
    QTemporaryDir living;
    QVERIFY(living.isValid());
    QTemporaryDir storeDir;
    QSettings settings(storeDir.filePath(QStringLiteral("recent.ini")), QSettings::IniFormat);
    RecentWorkspaces recents(settings);
    recents.remember(living.path());

    QTemporaryDir doomed;
    QVERIFY(doomed.isValid());
    const QString gone = doomed.path();
    recents.remember(gone);
    doomed.remove();
    QVERIFY(!QDir(gone).exists());

    const QStringList kept = recents.prune();
    QCOMPARE(kept.size(), 1);
    QVERIFY(RecentWorkspaces::samePath(kept.first(), living.path()));
    QCOMPARE(recents.entries().size(), 1);
}

void RecentWorkspacesTest::removeAndClear()
{
    QTemporaryDir first;
    QTemporaryDir second;
    QVERIFY(first.isValid());
    QVERIFY(second.isValid());
    QTemporaryDir storeDir;
    QSettings settings(storeDir.filePath(QStringLiteral("recent.ini")), QSettings::IniFormat);
    RecentWorkspaces recents(settings);
    recents.remember(first.path());
    recents.remember(second.path());
    recents.remove(first.path());
    QCOMPARE(recents.entries().size(), 1);
    QVERIFY(RecentWorkspaces::samePath(recents.entries().first(), second.path()));
    recents.clear();
    QVERIFY(recents.entries().isEmpty());
}

void RecentWorkspacesTest::ignoresNonexistent()
{
    QTemporaryDir storeDir;
    QSettings settings(storeDir.filePath(QStringLiteral("recent.ini")), QSettings::IniFormat);
    RecentWorkspaces recents(settings);
    recents.remember({});
    recents.remember(storeDir.filePath(QStringLiteral("missing-folder")));
    QVERIFY(recents.entries().isEmpty());
}

QTEST_GUILESS_MAIN(RecentWorkspacesTest)
#include "RecentWorkspacesTest.moc"
