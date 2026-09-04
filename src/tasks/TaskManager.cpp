#include "tasks/TaskManager.h"

#include "tasks/CMakeCommands.h"
#include "tasks/ProblemMatcher.h"
#include "tasks/TaskFile.h"
#include "tasks/TaskRunner.h"
#include "tasks/TaskVariables.h"

TaskManager::TaskManager(QObject *parent) : QObject(parent), m_runner(new TaskRunner(this))
{
    connect(m_runner, &TaskRunner::started, this, &TaskManager::started);
    connect(m_runner, &TaskRunner::outputReceived, this, &TaskManager::outputReceived);
    connect(m_runner, &TaskRunner::failed, this, &TaskManager::failed);
    connect(m_runner, &TaskRunner::finished, this, [this](int exitCode, const QString &output) {
        if (!m_currentMatcher.isEmpty()) {
            emit problemsMatched(
                matchTaskProblems(output, m_currentMatcher, m_workspace, m_currentCwd));
        }
        emit finished(exitCode);
    });
}

void TaskManager::setWorkspace(const QString &path)
{
    if (m_workspace == path) {
        return;
    }
    m_workspace = path;
    reload();
}

void TaskManager::setActiveFile(const QString &path)
{
    m_activeFile = path;
}

void TaskManager::reload()
{
    m_cmake = inspectCMakeWorkspace(m_workspace);
    const QStringList visible = m_cmake.visiblePresetNames();
    if (!visible.contains(m_preset)) {
        m_preset = visible.contains(QStringLiteral("debug"))
                       ? QStringLiteral("debug")
                       : (visible.isEmpty() ? QString() : visible.first());
    }
    rebuildTaskList();
    emit cmakeChanged();
    emit tasksChanged();
}

void TaskManager::rebuildTaskList()
{
    m_tasks = loadTasksFile(m_workspace);
    m_tasks += cmakeBuiltinTasks(m_cmake.detected);
}

TaskDefinition TaskManager::taskById(const QString &id) const
{
    for (const TaskDefinition &task : m_tasks) {
        if (task.id == id) {
            return task;
        }
    }
    return {};
}

bool TaskManager::isRunning() const
{
    return m_runner->isRunning();
}

void TaskManager::setPreset(const QString &preset)
{
    if (m_preset == preset) {
        return;
    }
    m_preset = preset;
    emit cmakeChanged();
}

void TaskManager::setTarget(const QString &target)
{
    m_target = target.trimmed();
}

void TaskManager::run(const QString &taskId)
{
    const TaskDefinition task = taskById(taskId);
    if (task.id.isEmpty()) {
        emit failed(tr("Unknown task"));
        return;
    }
    if (m_cmake.detected && task.kind != TaskKind::Custom && m_cmake.cmakeExecutable.isEmpty()) {
        emit failed(tr("cmake is not on PATH"));
        return;
    }
    const ProcessSpec spec = specFor(task);
    m_currentMatcher = spec.problemMatcher;
    m_currentCwd = spec.workingDirectory;
    m_runner->start(spec);
}

void TaskManager::runBuild()
{
    if (m_cmake.detected) {
        run(QStringLiteral("cmake.build"));
        return;
    }
    for (const TaskDefinition &task : m_tasks) {
        if (task.label.contains(QStringLiteral("build"), Qt::CaseInsensitive)) {
            run(task.id);
            return;
        }
    }
    if (!m_tasks.isEmpty()) {
        run(m_tasks.first().id);
        return;
    }
    emit failed(tr("No build task"));
}

void TaskManager::cancel()
{
    m_runner->cancel();
}

ProcessSpec TaskManager::specFor(const TaskDefinition &task) const
{
    switch (task.kind) {
    case TaskKind::CMakeConfigure:
        return cmakeConfigureSpec(m_cmake, m_preset);
    case TaskKind::CMakeBuild:
        return cmakeBuildSpec(m_cmake, m_preset, m_target);
    case TaskKind::CMakeClean:
        return cmakeCleanSpec(m_cmake, m_preset);
    case TaskKind::CMakeTest:
        return cmakeTestSpec(m_cmake, m_preset);
    case TaskKind::CMakeRun:
        return cmakeRunSpec(m_cmake, m_preset, m_target);
    case TaskKind::Custom:
        break;
    }

    const TaskContext context{m_workspace, m_activeFile};
    ProcessSpec spec;
    spec.title = task.label;
    spec.program = expandTaskVariables(task.command, context);
    spec.arguments = expandTaskVariables(task.args, context);
    spec.workingDirectory =
        expandTaskVariables(task.workingDirectory.isEmpty() ? QStringLiteral("${workspaceFolder}")
                                                            : task.workingDirectory,
                            context);
    spec.problemMatcher = task.problemMatcher;
    return spec;
}
