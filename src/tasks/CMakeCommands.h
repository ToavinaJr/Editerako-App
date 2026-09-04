#ifndef EDITERAKO_CMAKECOMMANDS_H
#define EDITERAKO_CMAKECOMMANDS_H

#include "tasks/CMakePresets.h"
#include "tasks/TaskDefinition.h"

[[nodiscard]] ProcessSpec cmakeConfigureSpec(const CMakeWorkspace &workspace, const QString &preset);
[[nodiscard]] ProcessSpec cmakeBuildSpec(const CMakeWorkspace &workspace, const QString &preset,
                                         const QString &target);
[[nodiscard]] ProcessSpec cmakeCleanSpec(const CMakeWorkspace &workspace, const QString &preset);
[[nodiscard]] ProcessSpec cmakeTestSpec(const CMakeWorkspace &workspace, const QString &preset);
[[nodiscard]] ProcessSpec cmakeRunSpec(const CMakeWorkspace &workspace, const QString &preset,
                                       const QString &target);
[[nodiscard]] QString cmakeTargetExecutable(const CMakeWorkspace &workspace, const QString &preset,
                                            const QString &target);
[[nodiscard]] QVector<TaskDefinition> cmakeBuiltinTasks(bool detected);

#endif
