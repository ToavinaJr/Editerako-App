#ifndef EDITERAKO_TASKMANAGER_H
#define EDITERAKO_TASKMANAGER_H

#include "tasks/CMakePresets.h"
#include "tasks/TaskDefinition.h"

#include <QObject>
#include <QVector>

class TaskRunner;

class TaskManager : public QObject
{
    Q_OBJECT

public:
    explicit TaskManager(QObject *parent = nullptr);

    void setWorkspace(const QString &path);
    void setActiveFile(const QString &path);
    void reload();

    [[nodiscard]] QString workspace() const { return m_workspace; }
    [[nodiscard]] QVector<TaskDefinition> tasks() const { return m_tasks; }
    [[nodiscard]] CMakeWorkspace cmake() const { return m_cmake; }
    [[nodiscard]] QString selectedPreset() const { return m_preset; }
    [[nodiscard]] QString selectedTarget() const { return m_target; }
    [[nodiscard]] bool isRunning() const;
    [[nodiscard]] TaskDefinition taskById(const QString &id) const;

    void setPreset(const QString &preset);
    void setTarget(const QString &target);
    void run(const QString &taskId);
    void runBuild();
    void cancel();

signals:
    void tasksChanged();
    void cmakeChanged();
    void started(const QString &title);
    void outputReceived(const QString &chunk);
    void finished(int exitCode);
    void failed(const QString &error);
    void problemsMatched(const QVector<TaskProblem> &problems);

private:
    [[nodiscard]] ProcessSpec specFor(const TaskDefinition &task) const;
    void rebuildTaskList();

    TaskRunner *m_runner = nullptr;
    QString m_workspace;
    QString m_activeFile;
    QString m_preset;
    QString m_target;
    QString m_currentMatcher;
    QString m_currentCwd;
    CMakeWorkspace m_cmake;
    QVector<TaskDefinition> m_tasks;
};

#endif
