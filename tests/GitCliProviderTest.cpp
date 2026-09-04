#include "scm/GitCliProvider.h"
#include "scm/GitProcess.h"

#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

class GitCliProviderTest : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void detectsNonRepository();
    void stageAndStatus();
};

void GitCliProviderTest::initTestCase()
{
    if (GitProcess::gitExecutable().isEmpty()) {
        QSKIP("git is not on PATH");
    }
}

static bool writeFile(const QString &path, const QByteArray &bytes)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    return file.write(bytes) == bytes.size();
}

static bool runGit(const QString &cwd, const QStringList &args)
{
    return GitProcess::run(cwd, args).ok();
}

void GitCliProviderTest::detectsNonRepository()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());

    GitCliProvider provider;
    QSignalSpy spy(&provider, &GitCliProvider::statusChanged);
    provider.setWorkspace(temp.path());
    QVERIFY(spy.wait(10000));
    const auto status = spy.last().at(0).value<ScmStatus>();
    QVERIFY(!status.isRepository);
}

void GitCliProviderTest::stageAndStatus()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    const QString root = temp.path();
    QVERIFY(runGit(root, {QStringLiteral("init")}));
    QVERIFY(runGit(root, {QStringLiteral("config"), QStringLiteral("user.email"),
                          QStringLiteral("test@example.com")}));
    QVERIFY(runGit(root, {QStringLiteral("config"), QStringLiteral("user.name"),
                          QStringLiteral("Editerako Test")}));

    const QString filePath = QDir(root).filePath(QStringLiteral("hello.txt"));
    QVERIFY(writeFile(filePath, "hello\n"));

    GitCliProvider provider;
    QSignalSpy spy(&provider, &GitCliProvider::statusChanged);
    provider.setWorkspace(root);
    QVERIFY(spy.wait(10000));

    ScmStatus status = spy.last().at(0).value<ScmStatus>();
    QVERIFY(status.isRepository);
    QVERIFY(!status.branch.isEmpty());
    QCOMPARE(status.changes.size(), 1);
    QCOMPARE(status.changes.first().state, ScmFileState::Untracked);

    spy.clear();
    provider.stage({filePath});
    QVERIFY(spy.wait(10000));
    status = spy.last().at(0).value<ScmStatus>();
    QCOMPARE(status.changes.size(), 1);
    QVERIFY(status.changes.first().staged);
    QCOMPARE(status.changes.first().state, ScmFileState::Added);
}

QTEST_GUILESS_MAIN(GitCliProviderTest)
#include "GitCliProviderTest.moc"
