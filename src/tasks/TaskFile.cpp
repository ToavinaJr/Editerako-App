#include "tasks/TaskFile.h"

#include "core/Logging.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

QString tasksFilePath(const QString &workspaceRoot)
{
    if (workspaceRoot.isEmpty()) {
        return {};
    }
    return QDir(workspaceRoot).filePath(QStringLiteral(".editerako/tasks.json"));
}

QVector<TaskDefinition> parseTasksJson(const QByteArray &json, QString *error)
{
    QVector<TaskDefinition> out;
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        if (error) {
            *error = parseError.errorString();
        }
        return out;
    }

    const QJsonArray tasks = doc.object().value(QStringLiteral("tasks")).toArray();
    int index = 0;
    for (const QJsonValue &value : tasks) {
        const QJsonObject obj = value.toObject();
        const QString label = obj.value(QStringLiteral("label")).toString().trimmed();
        const QString command = obj.value(QStringLiteral("command")).toString().trimmed();
        if (label.isEmpty() || command.isEmpty()) {
            qCWarning(lcTasks) << "Skipping task without label/command";
            ++index;
            continue;
        }

        TaskDefinition task;
        task.id = QStringLiteral("custom.%1").arg(index);
        task.label = label;
        task.command = command;
        task.workingDirectory = obj.value(QStringLiteral("workingDirectory")).toString();
        task.problemMatcher = obj.value(QStringLiteral("problemMatcher")).toString();
        const QJsonArray args = obj.value(QStringLiteral("args")).toArray();
        for (const QJsonValue &arg : args) {
            task.args.append(arg.toString());
        }
        out.append(task);
        ++index;
    }
    return out;
}

QVector<TaskDefinition> loadTasksFile(const QString &workspaceRoot, QString *error)
{
    const QString path = tasksFilePath(workspaceRoot);
    if (path.isEmpty() || !QFile::exists(path)) {
        return {};
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error) {
            *error = file.errorString();
        }
        return {};
    }
    return parseTasksJson(file.readAll(), error);
}
