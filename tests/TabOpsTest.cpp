#include "editor/TabOps.h"

#include <QtTest>

class TabOpsTest : public QObject
{
    Q_OBJECT

private slots:
    void closeToRightSkipsPinned();
    void closeToRightFromLastIsEmpty();
    void closeSavedSkipsDirtyAndPinned();
    void closeOthersKeepsPinned();
};

void TabOpsTest::closeToRightSkipsPinned()
{
    const QList<TabCloseFlags> tabs{
        {},
        {},
        {true, false},
        {},
    };
    QCOMPARE(tabIndicesCloseToRight(0, tabs), (QList<int>{1, 3}));
}

void TabOpsTest::closeToRightFromLastIsEmpty()
{
    const QList<TabCloseFlags> tabs{{}, {}};
    QVERIFY(tabIndicesCloseToRight(1, tabs).isEmpty());
    QVERIFY(tabIndicesCloseToRight(-1, tabs).isEmpty());
}

void TabOpsTest::closeSavedSkipsDirtyAndPinned()
{
    const QList<TabCloseFlags> tabs{
        {false, false},
        {false, true},
        {true, false},
        {false, false},
    };
    QCOMPARE(tabIndicesCloseSaved(tabs), (QList<int>{0, 3}));
}

void TabOpsTest::closeOthersKeepsPinned()
{
    const QList<TabCloseFlags> tabs{
        {true, false},
        {},
        {},
    };
    QCOMPARE(tabIndicesCloseOthers(1, tabs), (QList<int>{2}));
}

QTEST_GUILESS_MAIN(TabOpsTest)
#include "TabOpsTest.moc"
