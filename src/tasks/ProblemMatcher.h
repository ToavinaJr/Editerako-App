#ifndef EDITERAKO_PROBLEMMATCHER_H
#define EDITERAKO_PROBLEMMATCHER_H

#include "tasks/TaskDefinition.h"

#include <QString>
#include <QVector>

[[nodiscard]] QVector<TaskProblem> matchTaskProblems(const QString &output,
                                                     const QString &matcherId,
                                                     const QString &workspaceRoot,
                                                     const QString &workingDirectory);

#endif
