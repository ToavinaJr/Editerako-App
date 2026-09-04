#ifndef EDITERAKO_TASKFILE_H
#define EDITERAKO_TASKFILE_H

#include "tasks/TaskDefinition.h"

#include <QString>
#include <QVector>

[[nodiscard]] QString tasksFilePath(const QString &workspaceRoot);
[[nodiscard]] QVector<TaskDefinition> parseTasksJson(const QByteArray &json, QString *error = nullptr);
[[nodiscard]] QVector<TaskDefinition> loadTasksFile(const QString &workspaceRoot, QString *error = nullptr);

#endif
