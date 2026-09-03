#include "core/DiskChangePolicy.h"

#include <QtTest>

class DiskChangePolicyTest : public QObject
{
    Q_OBJECT

private slots:
    void deletedDirtyWarns();
    void deletedCleanCloses();
    void existingDirtyPrompts();
    void existingCleanReloads();
};

void DiskChangePolicyTest::deletedDirtyWarns()
{
    QCOMPARE(diskChangeAction(false, true), DiskChangeAction::WarnDeletedDirty);
}

void DiskChangePolicyTest::deletedCleanCloses()
{
    QCOMPARE(diskChangeAction(false, false), DiskChangeAction::CloseTab);
}

void DiskChangePolicyTest::existingDirtyPrompts()
{
    QCOMPARE(diskChangeAction(true, true), DiskChangeAction::PromptReload);
}

void DiskChangePolicyTest::existingCleanReloads()
{
    QCOMPARE(diskChangeAction(true, false), DiskChangeAction::Reload);
}

QTEST_GUILESS_MAIN(DiskChangePolicyTest)
#include "DiskChangePolicyTest.moc"
