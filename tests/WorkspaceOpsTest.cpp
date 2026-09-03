#include "project/WorkspaceOps.h"
#include "project/WorkspacePath.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QtTest>

class WorkspaceOpsTest : public QObject
{
    Q_OBJECT

private slots:
    void renameDuplicateCopyAndDelete();
    void refusesRootAndEscape();
};

static QString writeFile(const QString &path, const QByteArray &bytes = QByteArrayLiteral("hi"))
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return {};
    }
    file.write(bytes);
    return QDir::cleanPath(path);
}

void WorkspaceOpsTest::renameDuplicateCopyAndDelete()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    const QString root = QDir::cleanPath(QFileInfo(temp.path()).absoluteFilePath());
    QVERIFY(QDir(root).mkpath(QStringLiteral("src")));
    const QString src = writeFile(QDir(root).filePath(QStringLiteral("src/a.txt")));
    QVERIFY(!src.isEmpty());

    QString renamed;
    QVERIFY(renameInsideWorkspace(root, src, QStringLiteral("b.txt"), &renamed));
    QVERIFY(QFile::exists(renamed));
    QVERIFY(!QFile::exists(src));

    QString dup;
    QVERIFY(duplicateInsideWorkspace(root, renamed, &dup));
    QVERIFY(QFile::exists(dup));
    QVERIFY(dup.contains(QStringLiteral("copy")));

    QVERIFY(QDir(root).mkpath(QStringLiteral("dest")));
    QString copied;
    QVERIFY(copyInsideWorkspace(root, renamed, QDir(root).filePath(QStringLiteral("dest")), &copied));
    QVERIFY(QFile::exists(copied));

    QVERIFY(deleteInsideWorkspace(root, dup, WorkspaceDeleteMode::Permanent));
    QVERIFY(!QFile::exists(dup));
}

void WorkspaceOpsTest::refusesRootAndEscape()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    const QString root = QDir::cleanPath(QFileInfo(temp.path()).absoluteFilePath());
    QVERIFY(!deleteInsideWorkspace(root, root, WorkspaceDeleteMode::Permanent));
    QVERIFY(!renameInsideWorkspace(root, root, QStringLiteral("elsewhere")));

    const QString src = writeFile(QDir(root).filePath(QStringLiteral("a.txt")));
    QVERIFY(!renameInsideWorkspace(root, src, QStringLiteral("../x.txt")));
}

QTEST_GUILESS_MAIN(WorkspaceOpsTest)
#include "WorkspaceOpsTest.moc"
