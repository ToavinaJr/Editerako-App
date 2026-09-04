#include "tasks/TaskFile.h"
#include "tasks/TaskVariables.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

class TaskFileTest : public QObject
{
    Q_OBJECT

private slots:
    void expandsWorkspaceFolder();
    void parsesTasksJson();
    void skipsInvalidEntries();
    void missingFileIsEmpty();
};

void TaskFileTest::expandsWorkspaceFolder()
{
    const TaskContext context{QStringLiteral("C:/proj"), QStringLiteral("C:/proj/src/a.cpp")};
    QCOMPARE(expandTaskVariables(QStringLiteral("${workspaceFolder}/build"), context),
             QStringLiteral("C:/proj/build"));
    QCOMPARE(expandTaskVariables(QStringList{QStringLiteral("${file}")}, context).front(),
             QStringLiteral("C:/proj/src/a.cpp"));
}

void TaskFileTest::parsesTasksJson()
{
    const QByteArray json = R"({
        "version": 1,
        "tasks": [
            {
                "label": "Build",
                "command": "cmake",
                "args": ["--build", "build"],
                "workingDirectory": "${workspaceFolder}",
                "problemMatcher": "gcc"
            }
        ]
    })";
    const QVector<TaskDefinition> tasks = parseTasksJson(json);
    QCOMPARE(tasks.size(), 1);
    QCOMPARE(tasks.front().id, QStringLiteral("custom.0"));
    QCOMPARE(tasks.front().label, QStringLiteral("Build"));
    QCOMPARE(tasks.front().command, QStringLiteral("cmake"));
    QCOMPARE(tasks.front().args.size(), 2);
    QCOMPARE(tasks.front().problemMatcher, QStringLiteral("gcc"));
}

void TaskFileTest::skipsInvalidEntries()
{
    const QByteArray json = R"({"tasks":[{"label":"x"},{"command":"echo","label":"ok"}]})";
    const QVector<TaskDefinition> tasks = parseTasksJson(json);
    QCOMPARE(tasks.size(), 1);
    QCOMPARE(tasks.front().label, QStringLiteral("ok"));
    QCOMPARE(tasks.front().id, QStringLiteral("custom.1"));
}

void TaskFileTest::missingFileIsEmpty()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QVERIFY(loadTasksFile(dir.path()).isEmpty());

    QVERIFY(QDir(dir.path()).mkpath(QStringLiteral(".editerako")));
    QFile file(tasksFilePath(dir.path()));
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(R"({"tasks":[{"label":"Hi","command":"echo"}]})");
    file.close();
    QCOMPARE(loadTasksFile(dir.path()).size(), 1);
}

QTEST_GUILESS_MAIN(TaskFileTest)
#include "TaskFileTest.moc"
