#include "debug/LaunchFile.h"

#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

class LaunchFileTest : public QObject
{
    Q_OBJECT

private slots:
    void expandsVariables();
    void parsesLaunchJson();
    void resolvesGdbAdapter();
    void writesTemplateOnce();
};

void LaunchFileTest::expandsVariables()
{
    const LaunchContext context{QStringLiteral("C:/proj"), QStringLiteral("C:/proj/src/a.cpp")};
    QCOMPARE(expandLaunchVariables(QStringLiteral("${workspaceFolder}/build"), context),
             QStringLiteral("C:/proj/build"));
    QCOMPARE(expandLaunchVariables(QStringLiteral("${file}"), context),
             QStringLiteral("C:/proj/src/a.cpp"));
    QCOMPARE(expandLaunchVariables(QStringLiteral("${fileDirname}"), context),
             QStringLiteral("C:/proj/src"));
}

void LaunchFileTest::parsesLaunchJson()
{
    const QByteArray json = R"({
        "version": "0.2.0",
        "configurations": [
            {
                "name": "Launch",
                "type": "gdb",
                "request": "launch",
                "program": "${workspaceFolder}/a.exe",
                "cwd": "${workspaceFolder}",
                "args": ["--flag"]
            }
        ]
    })";
    const QVector<LaunchConfiguration> configs = parseLaunchJson(json);
    QCOMPARE(configs.size(), 1);
    QCOMPARE(configs.front().name, QStringLiteral("Launch"));
    QCOMPARE(configs.front().type, QStringLiteral("gdb"));
    QCOMPARE(configs.front().request, QStringLiteral("launch"));
    QCOMPARE(configs.front().arguments.value(QStringLiteral("program")).toString(),
             QStringLiteral("${workspaceFolder}/a.exe"));
    QCOMPARE(configs.front().arguments.value(QStringLiteral("args")).toArray().size(), 1);

    const LaunchConfiguration expanded =
        expandLaunchConfiguration(configs.front(),
                                  LaunchContext{QStringLiteral("C:/proj"), QString()});
    QCOMPARE(expanded.arguments.value(QStringLiteral("program")).toString(),
             QStringLiteral("C:/proj/a.exe"));
}

void LaunchFileTest::resolvesGdbAdapter()
{
    LaunchConfiguration config;
    config.type = QStringLiteral("gdb");
    QString error;
    QVERIFY(resolveDebugAdapter(&config, &error));
    QCOMPARE(config.adapterCommand, QStringLiteral("gdb"));
    QCOMPARE(config.adapterArgs, QStringList{QStringLiteral("--interpreter=dap")});
}

void LaunchFileTest::writesTemplateOnce()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QVERIFY(writeLaunchTemplate(dir.path()));
    QVERIFY(QFile::exists(launchFilePath(dir.path())));
    QCOMPARE(loadLaunchFile(dir.path()).size(), 1);
    QVERIFY(writeLaunchTemplate(dir.path()));
}

QTEST_GUILESS_MAIN(LaunchFileTest)
#include "LaunchFileTest.moc"
