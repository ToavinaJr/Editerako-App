#include "project/WorkspaceController.h"

#include "project/Workspace.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHeaderView>
#include <QAbstractItemView>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTreeWidget>
#include <QtTest>

class WorkspaceControllerTest : public QObject
{
    Q_OBJECT

private slots:
    void setRootPathReloadsAndEmits();
    void createFileAndFolder();
    void refreshIfContains();
    void rejectsPathTraversal();
    void explorerUsesCompactIndentAndHorizontalScroll();
};

void WorkspaceControllerTest::setRootPathReloadsAndEmits()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());

    QTreeWidget tree;
    WorkspaceController controller(&tree);
    QSignalSpy spy(&controller, &WorkspaceController::rootPathChanged);

    controller.setRootPath(temp.path());
    QCOMPARE(spy.count(), 1);
    QCOMPARE(controller.rootPath(), QDir::cleanPath(QFileInfo(temp.path()).absoluteFilePath()));
    QVERIFY(controller.workspace()->isValid());
    QCOMPARE(controller.targetDirectory(), controller.rootPath());
}

void WorkspaceControllerTest::createFileAndFolder()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());

    QTreeWidget tree;
    WorkspaceController controller(&tree);
    controller.setRootPath(temp.path());

    QString filePath;
    QVERIFY(controller.createEmptyFile(QStringLiteral("hello.txt"), &filePath));
    QVERIFY(QFile::exists(filePath));
    QVERIFY(!controller.createEmptyFile({}, nullptr));

    QString dirPath;
    QVERIFY(controller.createDirectory(QStringLiteral("nested/dir"), &dirPath));
    QVERIFY(QDir(dirPath).exists());
    QVERIFY(!controller.createDirectory({}, nullptr));
}

void WorkspaceControllerTest::refreshIfContains()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());

    QTreeWidget tree;
    WorkspaceController controller(&tree);
    controller.setRootPath(temp.path());

    const QString inside = QDir(temp.path()).filePath(QStringLiteral("in.txt"));
    QVERIFY(QFile(inside).open(QIODevice::WriteOnly));
    controller.refreshIfContains(inside);
    controller.refreshIfContains(QStringLiteral("/somewhere/else"));
}

void WorkspaceControllerTest::rejectsPathTraversal()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());

    QTreeWidget tree;
    WorkspaceController controller(&tree);
    controller.setRootPath(temp.path());

    QVERIFY(!controller.createEmptyFile(QStringLiteral("../escape.txt"), nullptr));
    QVERIFY(!controller.createDirectory(QStringLiteral("../escape-dir"), nullptr));
}

void WorkspaceControllerTest::explorerUsesCompactIndentAndHorizontalScroll()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());

    QTreeWidget tree;
    WorkspaceController controller(&tree);
    controller.setRootPath(temp.path());

    QCOMPARE(tree.indentation(), 12);
    QCOMPARE(tree.horizontalScrollBarPolicy(), Qt::ScrollBarAsNeeded);
    QCOMPARE(tree.horizontalScrollMode(), QAbstractItemView::ScrollPerPixel);
    QCOMPARE(tree.textElideMode(), Qt::ElideNone);
    QVERIFY(!tree.header()->stretchLastSection());
}

QTEST_MAIN(WorkspaceControllerTest)
#include "WorkspaceControllerTest.moc"
