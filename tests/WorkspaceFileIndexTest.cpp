#include "project/WorkspaceFileIndex.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

class WorkspaceFileIndexTest : public QObject
{
    Q_OBJECT

private slots:
    void collectRespectsExclusions();
    void destroyWhileIndexingDoesNotHang();
    void rebuildEmitsIndexUpdated();
};

void WorkspaceFileIndexTest::collectRespectsExclusions()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());

    QVERIFY(QDir(temp.path()).mkpath(QStringLiteral("src")));
    QVERIFY(QDir(temp.path()).mkpath(QStringLiteral("node_modules/pkg")));

    const QString keep = QDir(temp.path()).filePath(QStringLiteral("src/main.cpp"));
    const QString skip = QDir(temp.path()).filePath(QStringLiteral("node_modules/pkg/secret.js"));
    QVERIFY(QFile(keep).open(QIODevice::WriteOnly));
    QVERIFY(QFile(skip).open(QIODevice::WriteOnly));

    const QStringList files = collectWorkspaceFiles(
        temp.path(),
        {QStringLiteral("node_modules")});

    const QString keepAbs = QDir::cleanPath(QFileInfo(keep).absoluteFilePath());
    const QString skipAbs = QDir::cleanPath(QFileInfo(skip).absoluteFilePath());
    QVERIFY(files.contains(keepAbs));
    QVERIFY(!files.contains(skipAbs));
}

void WorkspaceFileIndexTest::destroyWhileIndexingDoesNotHang()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    const QString keep = QDir(temp.path()).filePath(QStringLiteral("hello.txt"));
    QVERIFY(QFile(keep).open(QIODevice::WriteOnly));

    {
        WorkspaceFileIndex index;
        index.setRootPath(temp.path());
        index.rebuild();
    }
    QVERIFY(true);
}

void WorkspaceFileIndexTest::rebuildEmitsIndexUpdated()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    const QString keep = QDir(temp.path()).filePath(QStringLiteral("hello.txt"));
    QVERIFY(QFile(keep).open(QIODevice::WriteOnly));

    WorkspaceFileIndex index;
    QSignalSpy spy(&index, &WorkspaceFileIndex::indexUpdated);
    index.setRootPath(temp.path());
    index.setExcludedNames({});
    index.rebuild();

    QTRY_VERIFY(spy.count() >= 1);
    const QString keepAbs = QDir::cleanPath(QFileInfo(keep).absoluteFilePath());
    QVERIFY(index.files().contains(keepAbs));
    QVERIFY(!index.isIndexing());
}

QTEST_GUILESS_MAIN(WorkspaceFileIndexTest)
#include "WorkspaceFileIndexTest.moc"
