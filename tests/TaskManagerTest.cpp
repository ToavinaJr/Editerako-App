#include "tasks/TaskManager.h"
#include "tasks/TaskRunner.h"

#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

class TaskManagerTest : public QObject
{
    Q_OBJECT

private slots:
    void loadsCustomAndCMakeTasks();
    void runnerEchoesOutput();
};

void TaskManagerTest::loadsCustomAndCMakeTasks()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QFile lists(QDir(dir.path()).filePath(QStringLiteral("CMakeLists.txt")));
    QVERIFY(lists.open(QIODevice::WriteOnly));
    lists.write("project(demo)\n");
    lists.close();
    QVERIFY(QDir(dir.path()).mkpath(QStringLiteral(".editerako")));
    QFile tasks(QDir(dir.path()).filePath(QStringLiteral(".editerako/tasks.json")));
    QVERIFY(tasks.open(QIODevice::WriteOnly));
    tasks.write(R"({"tasks":[{"label":"Hello","command":"echo"}]})");
    tasks.close();

    TaskManager manager;
    manager.setWorkspace(dir.path());
    QVERIFY(manager.cmake().detected);
    bool foundCustom = false;
    bool foundCMake = false;
    for (const TaskDefinition &task : manager.tasks()) {
        if (task.id == QStringLiteral("custom.0")) {
            foundCustom = true;
        }
        if (task.id == QStringLiteral("cmake.build")) {
            foundCMake = true;
        }
    }
    QVERIFY(foundCustom);
    QVERIFY(foundCMake);
}

void TaskManagerTest::runnerEchoesOutput()
{
    TaskRunner runner;
    QSignalSpy started(&runner, &TaskRunner::started);
    QSignalSpy finished(&runner, &TaskRunner::finished);
    ProcessSpec spec;
    spec.title = QStringLiteral("echo");
#ifdef Q_OS_WIN
    spec.program = QStringLiteral("cmd");
    spec.arguments = QStringList{QStringLiteral("/c"), QStringLiteral("echo hello-task")};
#else
    spec.program = QStringLiteral("echo");
    spec.arguments = QStringList{QStringLiteral("hello-task")};
#endif
    runner.start(spec);
    QVERIFY(finished.wait(10000));
    QCOMPARE(started.size(), 1);
    const QString output = finished.front().at(1).toString();
    QVERIFY(output.contains(QStringLiteral("hello-task")));
}

QTEST_GUILESS_MAIN(TaskManagerTest)
#include "TaskManagerTest.moc"
