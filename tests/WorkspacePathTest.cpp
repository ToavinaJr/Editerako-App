#include "project/WorkspacePath.h"

#include <QDir>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QtTest>

class WorkspacePathTest : public QObject
{
    Q_OBJECT

private slots:
    void rejectsTraversalAndAbsolute();
    void allowsNestedRelative();
    void resolveStaysInside();
    void containsPathUsesWorkspaceRoot();
};

void WorkspacePathTest::rejectsTraversalAndAbsolute()
{
    QVERIFY(!isSafeRelativePath({}));
    QVERIFY(!isSafeRelativePath(QStringLiteral("../secret.txt")));
    QVERIFY(!isSafeRelativePath(QStringLiteral("foo/../../etc/passwd")));
    QVERIFY(!isSafeRelativePath(QStringLiteral("/tmp/x")));
#ifdef Q_OS_WIN
    QVERIFY(!isSafeRelativePath(QStringLiteral("C:/Windows/notepad.exe")));
#endif
}

void WorkspacePathTest::allowsNestedRelative()
{
    QVERIFY(isSafeRelativePath(QStringLiteral("hello.txt")));
    QVERIFY(isSafeRelativePath(QStringLiteral("nested/dir")));
    QVERIFY(isSafeRelativePath(QStringLiteral("src/app/MainWindow.cpp")));
}

void WorkspacePathTest::resolveStaysInside()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    const QString root = QDir::cleanPath(QFileInfo(temp.path()).absoluteFilePath());

    const QString ok = resolveInsideWorkspace(root, root, QStringLiteral("src/a.txt"));
    QVERIFY(!ok.isEmpty());
    QVERIFY(isInsideWorkspace(root, ok));

    QVERIFY(resolveInsideWorkspace(root, root, QStringLiteral("../outside.txt")).isEmpty());
}

void WorkspacePathTest::containsPathUsesWorkspaceRoot()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    const QString root = QDir::cleanPath(QFileInfo(temp.path()).absoluteFilePath());
    QVERIFY(isInsideWorkspace(root, root));
    QVERIFY(isInsideWorkspace(root, QDir(root).filePath(QStringLiteral("a/b.txt"))));
    QVERIFY(!isInsideWorkspace(root, {}));
    QVERIFY(!isInsideWorkspace(root, QStringLiteral("/somewhere/else")));
}

QTEST_GUILESS_MAIN(WorkspacePathTest)
#include "WorkspacePathTest.moc"
