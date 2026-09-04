#ifndef EDITERAKO_TASKVARIABLES_H
#define EDITERAKO_TASKVARIABLES_H

#include <QString>
#include <QStringList>

struct TaskContext {
    QString workspaceFolder;
    QString file;
};

[[nodiscard]] QString expandTaskVariables(const QString &text, const TaskContext &context);
[[nodiscard]] QStringList expandTaskVariables(const QStringList &args, const TaskContext &context);

#endif
