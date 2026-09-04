#include "scm/GitParsers.h"

#include <QDir>
#include <QtTest>

class GitParsersTest : public QObject
{
    Q_OBJECT
private slots:
    void parsesBranchAndChanges();
    void parsesRenameAndUtf8();
    void parsesStagedAndUnstagedSameFile();
    void makesRelativePathsAbsolute();
};

void GitParsersTest::parsesBranchAndChanges()
{
    QByteArray input("## feature...origin/feature");
    input.append('\0'); input.append("M  staged.cpp"); input.append('\0');
    input.append(" M working.cpp"); input.append('\0'); input.append("?? new file.txt"); input.append('\0');
    const ScmStatus status = GitParsers::parseStatus(input);
    QCOMPARE(status.branch, QStringLiteral("feature"));
    QCOMPARE(status.changes.size(), 3);
    QVERIFY(status.changes.at(0).staged);
    QCOMPARE(status.changes.at(1).state, ScmFileState::Modified);
    QCOMPARE(status.changes.at(2).path, QStringLiteral("new file.txt"));
    QCOMPARE(status.changes.at(2).state, ScmFileState::Untracked);
}

void GitParsersTest::parsesRenameAndUtf8()
{
    QByteArray input("## main");
    input.append('\0'); input.append("R  nouveau.cpp"); input.append('\0');
    input.append("ancien.cpp"); input.append('\0');
    const ScmStatus status = GitParsers::parseStatus(input);
    QCOMPARE(status.changes.size(), 1);
    QCOMPARE(status.changes.first().path, QStringLiteral("nouveau.cpp"));
    QCOMPARE(status.changes.first().oldPath, QStringLiteral("ancien.cpp"));
    QCOMPARE(status.changes.first().state, ScmFileState::Renamed);
}

void GitParsersTest::parsesStagedAndUnstagedSameFile()
{
    QByteArray input("## main");
    input.append('\0');
    input.append("MM both.cpp");
    input.append('\0');
    const ScmStatus status = GitParsers::parseStatus(input);
    QCOMPARE(status.changes.size(), 2);
    QVERIFY(status.changes.at(0).staged);
    QVERIFY(!status.changes.at(1).staged);
    QCOMPARE(status.changes.at(0).state, ScmFileState::Modified);
    QCOMPARE(status.changes.at(1).state, ScmFileState::Modified);
}

void GitParsersTest::makesRelativePathsAbsolute()
{
    ScmStatus status;
    ScmChange change;
    change.path = QStringLiteral("src/main.cpp");
    status.changes.append(change);
    GitParsers::makePathsAbsolute(status, QStringLiteral("C:/repo"));
    QCOMPARE(status.repositoryRoot, QStringLiteral("C:/repo"));
    QCOMPARE(status.changes.first().path, QDir::cleanPath(QStringLiteral("C:/repo/src/main.cpp")));
}

QTEST_GUILESS_MAIN(GitParsersTest)
#include "GitParsersTest.moc"
