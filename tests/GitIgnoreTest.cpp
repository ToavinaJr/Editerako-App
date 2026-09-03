#include "project/GitIgnore.h"

#include <QtTest>

class GitIgnoreTest : public QObject
{
    Q_OBJECT

private slots:
    void globIncludeExclude();
    void gitignorePatterns();
    void alwaysIgnoresGitDir();
};

void GitIgnoreTest::globIncludeExclude()
{
    QVERIFY(globMatches({}, QStringLiteral("src/a.cpp")));
    QVERIFY(globMatches(QStringLiteral("*.cpp"), QStringLiteral("src/a.cpp")));
    QVERIFY(!globMatches(QStringLiteral("*.h"), QStringLiteral("src/a.cpp")));
    QVERIFY(globMatches(QStringLiteral("*.cpp, *.h"), QStringLiteral("src/a.h")));
}

void GitIgnoreTest::gitignorePatterns()
{
    const GitIgnore ignore = GitIgnore::fromText(
        QStringLiteral("# comment\n*.log\n!keep.log\nbuild/\n/secret.txt\n"));
    QVERIFY(ignore.isIgnored(QStringLiteral("debug.log"), false));
    QVERIFY(ignore.isIgnored(QStringLiteral("src/debug.log"), false));
    QVERIFY(!ignore.isIgnored(QStringLiteral("keep.log"), false));
    QVERIFY(ignore.isIgnored(QStringLiteral("build"), true));
    QVERIFY(ignore.isIgnored(QStringLiteral("build/out.o"), false));
    QVERIFY(ignore.isIgnored(QStringLiteral("secret.txt"), false));
    QVERIFY(!ignore.isIgnored(QStringLiteral("src/secret.txt"), false));
    QVERIFY(!ignore.isIgnored(QStringLiteral("src/main.cpp"), false));
}

void GitIgnoreTest::alwaysIgnoresGitDir()
{
    const GitIgnore ignore = GitIgnore::fromText({});
    QVERIFY(ignore.isIgnored(QStringLiteral(".git"), true));
    QVERIFY(ignore.isIgnored(QStringLiteral(".git/config"), false));
}

QTEST_GUILESS_MAIN(GitIgnoreTest)
#include "GitIgnoreTest.moc"
