#include "tasks/CMakeCommands.h"

#include <QDir>

namespace {

ProcessSpec makeSpec(const QString &title, const QString &program, const QStringList &args,
                     const QString &cwd)
{
    ProcessSpec spec;
    spec.title = title;
    spec.program = program;
    spec.arguments = args;
    spec.workingDirectory = cwd;
    spec.problemMatcher = QStringLiteral("gcc");
    return spec;
}

} // namespace

ProcessSpec cmakeConfigureSpec(const CMakeWorkspace &workspace, const QString &preset)
{
    QStringList args;
    if (!preset.isEmpty()) {
        args << QStringLiteral("--preset") << preset;
    } else {
        args << QStringLiteral("-S") << workspace.sourceDir << QStringLiteral("-B")
             << workspace.binaryDirFor({});
    }
    return makeSpec(QStringLiteral("CMake: Configure"), workspace.cmakeExecutable, args,
                    workspace.sourceDir);
}

ProcessSpec cmakeBuildSpec(const CMakeWorkspace &workspace, const QString &preset,
                           const QString &target)
{
    QStringList args{QStringLiteral("--build")};
    if (!preset.isEmpty()) {
        args << QStringLiteral("--preset") << preset;
    } else {
        args << workspace.binaryDirFor({});
    }
    if (!target.isEmpty()) {
        args << QStringLiteral("--target") << target;
    }
    return makeSpec(QStringLiteral("CMake: Build"), workspace.cmakeExecutable, args,
                    workspace.sourceDir);
}

ProcessSpec cmakeCleanSpec(const CMakeWorkspace &workspace, const QString &preset)
{
    ProcessSpec spec = cmakeBuildSpec(workspace, preset, QStringLiteral("clean"));
    spec.title = QStringLiteral("CMake: Clean");
    return spec;
}

ProcessSpec cmakeTestSpec(const CMakeWorkspace &workspace, const QString &preset)
{
    const QString binaryDir = workspace.binaryDirFor(preset);
    if (workspace.ctestExecutable.isEmpty()) {
        return makeSpec(QStringLiteral("CMake: Test"), workspace.cmakeExecutable,
                        {QStringLiteral("--build"), binaryDir, QStringLiteral("--target"),
                         QStringLiteral("test")},
                        workspace.sourceDir);
    }
    QStringList args;
    if (!preset.isEmpty()) {
        args << QStringLiteral("--preset") << preset;
    } else {
        args << QStringLiteral("--test-dir") << binaryDir;
    }
    args << QStringLiteral("--output-on-failure");
    return makeSpec(QStringLiteral("CMake: Test"), workspace.ctestExecutable, args,
                    workspace.sourceDir);
}

ProcessSpec cmakeRunSpec(const CMakeWorkspace &workspace, const QString &preset,
                         const QString &target)
{
    ProcessSpec spec;
    spec.title = QStringLiteral("CMake: Run");
    spec.program = cmakeTargetExecutable(workspace, preset, target);
    spec.workingDirectory = workspace.binaryDirFor(preset);
    spec.detach = true;
    return spec;
}

QString cmakeTargetExecutable(const CMakeWorkspace &workspace, const QString &preset,
                              const QString &target)
{
    if (target.isEmpty()) {
        return {};
    }
    QString name = target;
#ifdef Q_OS_WIN
    if (!name.endsWith(QStringLiteral(".exe"), Qt::CaseInsensitive)) {
        name += QStringLiteral(".exe");
    }
#endif
    return QDir(workspace.binaryDirFor(preset)).filePath(name);
}

QVector<TaskDefinition> cmakeBuiltinTasks(bool detected)
{
    if (!detected) {
        return {};
    }
    const struct {
        const char *id;
        const char *label;
        TaskKind kind;
    } builtins[] = {
        {"cmake.configure", "CMake: Configure", TaskKind::CMakeConfigure},
        {"cmake.build", "CMake: Build", TaskKind::CMakeBuild},
        {"cmake.clean", "CMake: Clean", TaskKind::CMakeClean},
        {"cmake.test", "CMake: Test", TaskKind::CMakeTest},
        {"cmake.run", "CMake: Run", TaskKind::CMakeRun},
    };
    QVector<TaskDefinition> out;
    for (const auto &item : builtins) {
        TaskDefinition task;
        task.id = QString::fromLatin1(item.id);
        task.label = QString::fromLatin1(item.label);
        task.kind = item.kind;
        task.problemMatcher = QStringLiteral("gcc");
        out.append(task);
    }
    return out;
}
