#include "tasks/TaskVariables.h"

#include <QDir>

QString expandTaskVariables(const QString &text, const TaskContext &context)
{
    QString out = text;
    const QString folder = QDir::fromNativeSeparators(context.workspaceFolder);
    out.replace(QStringLiteral("${workspaceFolder}"), folder);
    out.replace(QStringLiteral("${workspaceRoot}"), folder);
    out.replace(QStringLiteral("${file}"), QDir::fromNativeSeparators(context.file));
    return out;
}

QStringList expandTaskVariables(const QStringList &args, const TaskContext &context)
{
    QStringList out;
    out.reserve(args.size());
    for (const QString &arg : args) {
        out.append(expandTaskVariables(arg, context));
    }
    return out;
}
