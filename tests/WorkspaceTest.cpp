#include "project/Workspace.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

class WorkspaceTest : public QObject
{
    Q_OBJECT

private slots:
    void emptyRootIsInvalid();
    void setRootPathNormalizesAndEmits();
    void containsPath();
    void excludedNames();
    void createFileAndFolder();
    void listEntriesSkipsExcluded();
};

void WorkspaceTest::emptyRootIsInvalid()
{
    Workspace workspace;
    QVERIFY(!workspace.isValid());
    QVERIFY(workspace.rootPath().isEmpty());
}

void WorkspaceTest::setRootPathNormalizesAndEmits()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());

    Workspace workspace;
    QSignalSpy spy(&workspace, &Workspace::rootPathChanged);
    workspace.setRootPath(temp.path());
    QCOMPARE(spy.count(), 1);
    QCOMPARE(workspace.rootPath(), QDir::cleanPath(QFileInfo(temp.path()).absoluteFilePath()));
    QVERIFY(workspace.isValid());

    workspace.setRootPath(temp.path());
    QCOMPARE(spy.count(), 1);
}

void WorkspaceTest::containsPath()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());

    Workspace workspace;
    workspace.setRootPath(temp.path());

    const QString inside = QDir(temp.path()).filePath(QStringLiteral("src/main.cpp"));
    QVERIFY(workspace.containsPath(workspace.rootPath()));
    QVERIFY(workspace.containsPath(inside));
    QVERIFY(!workspace.containsPath({}));
    QVERIFY(!workspace.containsPath(QStringLiteral("/somewhere/else")));
}

void WorkspaceTest::excludedNames()
{
    Workspace workspace;
    QVERIFY(workspace.isExcludedName(QStringLiteral(".git")));
    QVERIFY(workspace.isExcludedName(QStringLiteral("NODE_MODULES")));
    QVERIFY(workspace.isExcludedName(QStringLiteral("build")));
    QVERIFY(!workspace.isExcludedName({}));
    QVERIFY(!workspace.isExcludedName(QStringLiteral("src")));
}

void WorkspaceTest::createFileAndFolder()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());

    QString filePath;
    QVERIFY(Workspace::createEmptyFile(temp.path(), QStringLiteral("hello.txt"), &filePath));
    QVERIFY(QFile::exists(filePath));
    QVERIFY(!Workspace::createEmptyFile(temp.path(), {}, nullptr));

    QString dirPath;
    QVERIFY(Workspace::createDirectory(temp.path(), QStringLiteral("nested/dir"), &dirPath));
    QVERIFY(QDir(dirPath).exists());
    QVERIFY(!Workspace::createDirectory(temp.path(), {}, nullptr));
}

void WorkspaceTest::listEntriesSkipsExcluded()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());

    QVERIFY(QDir(temp.path()).mkdir(QStringLiteral(".git")));
    QVERIFY(QDir(temp.path()).mkdir(QStringLiteral("src")));
    QFile file(QDir(temp.path()).filePath(QStringLiteral("readme.txt")));
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.close();

    Workspace workspace;
    workspace.setRootPath(temp.path());
    const QList<Workspace::Entry> entries = workspace.listEntries(temp.path());

    QStringList names;
    for (const Workspace::Entry &entry : entries) {
        names.append(entry.name);
    }
    QVERIFY(names.contains(QStringLiteral("src")));
    QVERIFY(names.contains(QStringLiteral("readme.txt")));
    QVERIFY(!names.contains(QStringLiteral(".git")));
}

QTEST_GUILESS_MAIN(WorkspaceTest)
#include "WorkspaceTest.moc"
