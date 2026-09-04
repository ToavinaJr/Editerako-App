#include "terminal/ShellProfiles.h"

#include <QFileInfo>
#include <QStringList>
#include <QtTest>

class ShellProfilesTest : public QObject
{
    Q_OBJECT

private slots:
    void defaultShellIsNotEmpty();
    void commandArgumentsMatchShell();
    void detectAtLeastOneProfileOnThisPlatform();
};

void ShellProfilesTest::defaultShellIsNotEmpty()
{
    QVERIFY(!defaultShellPath().isEmpty());
}

void ShellProfilesTest::commandArgumentsMatchShell()
{
#ifdef Q_OS_WIN
    QCOMPARE(shellCommandArguments(QStringLiteral("cmd.exe"), QStringLiteral("echo hi")),
             (QStringList{QStringLiteral("/d"), QStringLiteral("/c"), QStringLiteral("echo hi")}));
    QCOMPARE(shellCommandArguments(QStringLiteral("powershell.exe"), QStringLiteral("echo hi")),
             (QStringList{QStringLiteral("-NoProfile"), QStringLiteral("-Command"),
                          QStringLiteral("echo hi")}));
    QCOMPARE(shellCommandArguments(QStringLiteral("pwsh"), QStringLiteral("Get-Date")),
             (QStringList{QStringLiteral("-NoProfile"), QStringLiteral("-Command"),
                          QStringLiteral("Get-Date")}));
#else
    QCOMPARE(shellCommandArguments(QStringLiteral("/bin/bash"), QStringLiteral("echo hi")),
             (QStringList{QStringLiteral("-c"), QStringLiteral("echo hi")}));
#endif
}

void ShellProfilesTest::detectAtLeastOneProfileOnThisPlatform()
{
    const QVector<TerminalProfile> profiles = detectShellProfiles();
    QVERIFY(!profiles.isEmpty());
    QVERIFY(!profiles.front().shell.isEmpty());
    QVERIFY(QFileInfo::exists(profiles.front().shell)
            || QFileInfo(profiles.front().shell).isExecutable());
}

QTEST_GUILESS_MAIN(ShellProfilesTest)
#include "ShellProfilesTest.moc"
