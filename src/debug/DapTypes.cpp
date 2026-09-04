#include "debug/DapTypes.h"

#include <QDir>
#include <QFileInfo>
#include <QJsonArray>

QString dapSourcePath(const QJsonObject &source)
{
    const QString path = source.value(QStringLiteral("path")).toString();
    if (!path.isEmpty()) {
        return path;
    }
    return source.value(QStringLiteral("name")).toString();
}

DapStoppedEvent dapStoppedFromJson(const QJsonObject &body)
{
    DapStoppedEvent event;
    event.reason = body.value(QStringLiteral("reason")).toString();
    event.description = body.value(QStringLiteral("description")).toString();
    event.text = body.value(QStringLiteral("text")).toString();
    event.threadId = body.value(QStringLiteral("threadId")).toInt(1);
    event.allThreadsStopped = body.value(QStringLiteral("allThreadsStopped")).toBool();
    return event;
}

QVector<DapStackFrame> dapStackFramesFromJson(const QJsonObject &body)
{
    QVector<DapStackFrame> frames;
    const QJsonArray array = body.value(QStringLiteral("stackFrames")).toArray();
    frames.reserve(array.size());
    for (const QJsonValue &value : array) {
        const QJsonObject obj = value.toObject();
        DapStackFrame frame;
        frame.id = obj.value(QStringLiteral("id")).toInt();
        frame.name = obj.value(QStringLiteral("name")).toString();
        frame.sourcePath = dapSourcePath(obj.value(QStringLiteral("source")).toObject());
        frame.line = obj.value(QStringLiteral("line")).toInt();
        frame.column = obj.value(QStringLiteral("column")).toInt();
        frames.append(frame);
    }
    return frames;
}

QVector<DapScope> dapScopesFromJson(const QJsonObject &body)
{
    QVector<DapScope> scopes;
    const QJsonArray array = body.value(QStringLiteral("scopes")).toArray();
    scopes.reserve(array.size());
    for (const QJsonValue &value : array) {
        const QJsonObject obj = value.toObject();
        DapScope scope;
        scope.name = obj.value(QStringLiteral("name")).toString();
        scope.variablesReference = obj.value(QStringLiteral("variablesReference")).toInt();
        scope.expensive = obj.value(QStringLiteral("expensive")).toBool();
        scopes.append(scope);
    }
    return scopes;
}

QVector<DapVariable> dapVariablesFromJson(const QJsonObject &body)
{
    QVector<DapVariable> variables;
    const QJsonArray array = body.value(QStringLiteral("variables")).toArray();
    variables.reserve(array.size());
    for (const QJsonValue &value : array) {
        const QJsonObject obj = value.toObject();
        DapVariable variable;
        variable.name = obj.value(QStringLiteral("name")).toString();
        variable.value = obj.value(QStringLiteral("value")).toString();
        variable.type = obj.value(QStringLiteral("type")).toString();
        variable.variablesReference = obj.value(QStringLiteral("variablesReference")).toInt();
        variables.append(variable);
    }
    return variables;
}

QString dapNormalizePath(const QString &path)
{
    if (path.isEmpty()) {
        return {};
    }
    const QFileInfo info(path);
    const QString canonical = info.canonicalFilePath();
    if (!canonical.isEmpty()) {
        return canonical;
    }
    return QDir::cleanPath(info.absoluteFilePath());
}
