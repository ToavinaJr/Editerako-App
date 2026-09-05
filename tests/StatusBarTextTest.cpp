#include "editor/StatusBarText.h"

#include <QtTest>

class StatusBarTextTest : public QObject
{
    Q_OBJECT

private slots:
    void positionIsOneBased();
    void indentModeSpacesOrTabs();
    void tabSizeIsClamped();
    void problemsNoneErrorsWarningsAndBoth();
};

void StatusBarTextTest::positionIsOneBased()
{
    QCOMPARE(statusBarPositionLabel(3, 8), QStringLiteral("Ln 3, Col 8"));
    QCOMPARE(statusBarPositionLabel(0, 0), QStringLiteral("Ln 1, Col 1"));
}

void StatusBarTextTest::indentModeSpacesOrTabs()
{
    QCOMPARE(statusBarIndentModeLabel(true), QStringLiteral("Spaces"));
    QCOMPARE(statusBarIndentModeLabel(false), QStringLiteral("Tabs"));
}

void StatusBarTextTest::tabSizeIsClamped()
{
    QCOMPARE(statusBarTabSizeLabel(4), QStringLiteral("Tab Size: 4"));
    QCOMPARE(statusBarTabSizeLabel(0), QStringLiteral("Tab Size: 1"));
    QCOMPARE(statusBarTabSizeLabel(99), QStringLiteral("Tab Size: 16"));
}

void StatusBarTextTest::problemsNoneErrorsWarningsAndBoth()
{
    QCOMPARE(statusBarProblemsLabel(0, 0), QStringLiteral("No Problems"));
    QCOMPARE(statusBarProblemsLabel(2, 0), QStringLiteral("2 Errors"));
    QCOMPARE(statusBarProblemsLabel(0, 3), QStringLiteral("3 Warnings"));
    QCOMPARE(statusBarProblemsLabel(1, 4), QStringLiteral("1 Errors, 4 Warnings"));
}

QTEST_GUILESS_MAIN(StatusBarTextTest)
#include "StatusBarTextTest.moc"
