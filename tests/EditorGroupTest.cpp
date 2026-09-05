#include "editor/EditorGroup.h"

#include <QCoreApplication>
#include <QLabel>
#include <QMouseEvent>
#include <QSignalSpy>
#include <QTabBar>
#include <QTabWidget>
#include <QtTest>

namespace {

void showGroup(EditorGroup *group)
{
    group->resize(480, 80);
    group->show();
}

} // namespace

class EditorGroupTest : public QObject
{
    Q_OBJECT

private slots:
    void pinMovesToLeft();
    void previewIsExclusive();
    void pinClearsPreview();
    void middleClickDoesNotClosePinned();
    void middleClickClosesUnpinned();
};

void EditorGroupTest::pinMovesToLeft()
{
    EditorGroup group;
    auto *first = new QLabel(QStringLiteral("a"));
    auto *second = new QLabel(QStringLiteral("b"));
    auto *third = new QLabel(QStringLiteral("c"));
    group.tabWidget()->addTab(first, QStringLiteral("a"));
    group.tabWidget()->addTab(second, QStringLiteral("b"));
    group.tabWidget()->addTab(third, QStringLiteral("c"));

    group.setPinned(2, true);
    QCOMPARE(group.tabWidget()->widget(0), third);
    QVERIFY(group.isPinned(0));
    QCOMPARE(group.pinnedCount(), 1);
}

void EditorGroupTest::previewIsExclusive()
{
    EditorGroup group;
    group.tabWidget()->addTab(new QLabel(QStringLiteral("a")), QStringLiteral("a"));
    group.tabWidget()->addTab(new QLabel(QStringLiteral("b")), QStringLiteral("b"));

    group.setPreview(0, true);
    group.setPreview(1, true);
    QCOMPARE(group.previewIndex(), 1);
    QVERIFY(!group.isPreview(0));
    QVERIFY(group.isPreview(1));
}

void EditorGroupTest::pinClearsPreview()
{
    EditorGroup group;
    group.tabWidget()->addTab(new QLabel(QStringLiteral("a")), QStringLiteral("a"));
    group.setPreview(0, true);
    group.setPinned(0, true);
    QVERIFY(group.isPinned(0));
    QVERIFY(!group.isPreview(0));
}

void EditorGroupTest::middleClickDoesNotClosePinned()
{
    EditorGroup group;
    showGroup(&group);
    group.tabWidget()->addTab(new QLabel(QStringLiteral("a")), QStringLiteral("a"));
    group.setPinned(0, true);

    QSignalSpy spy(&group, &EditorGroup::tabCloseRequested);
    QTabBar *bar = group.tabWidget()->tabBar();
    const QPoint center = bar->tabRect(0).center();
    QMouseEvent press(QEvent::MouseButtonPress,
                      center,
                      bar->mapToGlobal(center),
                      Qt::MiddleButton,
                      Qt::MiddleButton,
                      Qt::NoModifier);
    QCoreApplication::sendEvent(bar, &press);
    QCOMPARE(spy.count(), 0);
}

void EditorGroupTest::middleClickClosesUnpinned()
{
    EditorGroup group;
    showGroup(&group);
    group.tabWidget()->addTab(new QLabel(QStringLiteral("a")), QStringLiteral("a"));

    QSignalSpy spy(&group, &EditorGroup::tabCloseRequested);
    QTabBar *bar = group.tabWidget()->tabBar();
    const QPoint center = bar->tabRect(0).center();
    QMouseEvent press(QEvent::MouseButtonPress,
                      center,
                      bar->mapToGlobal(center),
                      Qt::MiddleButton,
                      Qt::MiddleButton,
                      Qt::NoModifier);
    QCoreApplication::sendEvent(bar, &press);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toInt(), 0);
}

QTEST_MAIN(EditorGroupTest)
#include "EditorGroupTest.moc"
