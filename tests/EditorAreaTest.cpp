#include "editor/EditorArea.h"
#include "editor/EditorGroup.h"

#include <QSplitter>
#include <QtTest>

class EditorAreaTest : public QObject
{
    Q_OBJECT

private slots:
    void startsWithOneGroup();
    void splitRightThenUnwrap();
    void nestedSplitDown();
};

void EditorAreaTest::startsWithOneGroup()
{
    EditorArea area;
    auto *group = new EditorGroup;
    area.setInitialGroup(group);
    QCOMPARE(area.groupCount(), 1);
    QCOMPARE(area.groups().front(), group);
}

void EditorAreaTest::splitRightThenUnwrap()
{
    EditorArea area;
    auto *left = new EditorGroup;
    auto *right = new EditorGroup;
    area.setInitialGroup(left);
    area.split(left, right, Qt::Horizontal);
    QCOMPARE(area.groupCount(), 2);

    auto *splitter = qobject_cast<QSplitter *>(left->parentWidget());
    QVERIFY(splitter);
    QCOMPARE(splitter->orientation(), Qt::Horizontal);
    QCOMPARE(splitter->count(), 2);

    area.removeGroup(right);
    QCOMPARE(area.groupCount(), 1);
    QCOMPARE(area.groups().front(), left);
    QVERIFY(!qobject_cast<QSplitter *>(left->parentWidget()));
}

void EditorAreaTest::nestedSplitDown()
{
    EditorArea area;
    auto *left = new EditorGroup;
    auto *right = new EditorGroup;
    auto *bottom = new EditorGroup;
    area.setInitialGroup(left);
    area.split(left, right, Qt::Horizontal);
    area.split(right, bottom, Qt::Vertical);
    QCOMPARE(area.groupCount(), 3);

    auto *inner = qobject_cast<QSplitter *>(right->parentWidget());
    QVERIFY(inner);
    QCOMPARE(inner->orientation(), Qt::Vertical);

    area.removeGroup(bottom);
    QCOMPARE(area.groupCount(), 2);
}

QTEST_MAIN(EditorAreaTest)
#include "EditorAreaTest.moc"
