#include "tasks/CMakeCommands.h"
#include "tasks/CMakePresets.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

class CMakePresetsTest : public QObject
{
    Q_OBJECT

private slots:
    void parsesVisiblePresetsAndBinaryDir();
    void inspectsWorkspace();
    void buildsCliSpecs();
};

void CMakePresetsTest::parsesVisiblePresetsAndBinaryDir()
{
    const QByteArray json = R"({
      "configurePresets": [
        {"name": "base", "hidden": true, "binaryDir": "${sourceDir}/build/${presetName}"},
        {"name": "debug", "inherits": "base", "displayName": "Debug"},
        {"name": "release", "inherits": "base"}
      ]
    })";
    CMakeWorkspace workspace;
    workspace.sourceDir = QStringLiteral("C:/src");
    workspace.configurePresets = parseConfigurePresets(json);
    QCOMPARE(workspace.visiblePresetNames(),
             QStringList({QStringLiteral("debug"), QStringLiteral("release")}));
    QCOMPARE(workspace.binaryDirFor(QStringLiteral("debug")),
             QStringLiteral("C:/src/build/debug"));
}

void CMakePresetsTest::inspectsWorkspace()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QVERIFY(!inspectCMakeWorkspace(dir.path()).detected);

    QFile lists(QDir(dir.path()).filePath(QStringLiteral("CMakeLists.txt")));
    QVERIFY(lists.open(QIODevice::WriteOnly));
    lists.write("project(demo)\n");
    lists.close();
    QVERIFY(inspectCMakeWorkspace(dir.path()).detected);
}

void CMakePresetsTest::buildsCliSpecs()
{
    CMakeWorkspace workspace;
    workspace.detected = true;
    workspace.sourceDir = QStringLiteral("C:/src");
    workspace.cmakeExecutable = QStringLiteral("cmake");
    workspace.ctestExecutable = QStringLiteral("ctest");
    CMakePresetInfo debug;
    debug.name = QStringLiteral("debug");
    debug.binaryDir = QStringLiteral("${sourceDir}/build/${presetName}");
    workspace.configurePresets.append(debug);

    const ProcessSpec configure = cmakeConfigureSpec(workspace, QStringLiteral("debug"));
    QCOMPARE(configure.arguments, QStringList({QStringLiteral("--preset"), QStringLiteral("debug")}));
    const ProcessSpec build = cmakeBuildSpec(workspace, QStringLiteral("debug"), QStringLiteral("Editerako"));
    QVERIFY(build.arguments.contains(QStringLiteral("--target")));
    const ProcessSpec test = cmakeTestSpec(workspace, QStringLiteral("debug"));
    QCOMPARE(test.program, QStringLiteral("ctest"));
    QCOMPARE(cmakeBuiltinTasks(true).size(), 5);
    QVERIFY(cmakeBuiltinTasks(false).isEmpty());
}

QTEST_GUILESS_MAIN(CMakePresetsTest)
#include "CMakePresetsTest.moc"
