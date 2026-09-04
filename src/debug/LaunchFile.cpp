#include "debug/LaunchFile.h"

#include "core/Logging.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace {

QStringList jsonStringList(const QJsonValue &value)
{
    QStringList out;
    if (value.isString() && !value.toString().isEmpty()) {
        out.append(value.toString());
        return out;
    }
    const QJsonArray array = value.toArray();
    for (const QJsonValue &item : array) {
        out.append(item.toString());
    }
    return out;
}

bool isGdbType(const QString &type)
{
    return type == QLatin1String("gdb") || type == QLatin1String("cppdbg");
}

} // namespace

QString launchFilePath(const QString &workspaceRoot)
{
    if (workspaceRoot.isEmpty()) {
        return {};
    }
    return QDir(workspaceRoot).filePath(QStringLiteral(".editerako/launch.json"));
}

QString expandLaunchVariables(const QString &text, const LaunchContext &context)
{
    QString out = text;
    const QString folder = QDir::fromNativeSeparators(context.workspaceFolder);
    const QString file = QDir::fromNativeSeparators(context.file);
    const QString fileDir =
        file.isEmpty() ? QString() : QDir::fromNativeSeparators(QFileInfo(file).path());
    out.replace(QStringLiteral("${workspaceFolder}"), folder);
    out.replace(QStringLiteral("${workspaceRoot}"), folder);
    out.replace(QStringLiteral("${file}"), file);
    out.replace(QStringLiteral("${fileDirname}"), fileDir);
    return out;
}

QJsonValue expandLaunchValue(const QJsonValue &value, const LaunchContext &context)
{
    if (value.isString()) {
        return expandLaunchVariables(value.toString(), context);
    }
    if (value.isArray()) {
        QJsonArray out;
        const QJsonArray array = value.toArray();
        for (const QJsonValue &item : array) {
            out.append(expandLaunchValue(item, context));
        }
        return out;
    }
    if (value.isObject()) {
        QJsonObject out;
        const QJsonObject obj = value.toObject();
        for (auto it = obj.begin(); it != obj.end(); ++it) {
            out.insert(it.key(), expandLaunchValue(it.value(), context));
        }
        return out;
    }
    return value;
}

LaunchConfiguration expandLaunchConfiguration(LaunchConfiguration config,
                                              const LaunchContext &context)
{
    config.name = expandLaunchVariables(config.name, context);
    config.adapterCommand = expandLaunchVariables(config.adapterCommand, context);
    for (QString &arg : config.adapterArgs) {
        arg = expandLaunchVariables(arg, context);
    }
    config.arguments = expandLaunchValue(config.arguments, context).toObject();
    return config;
}

bool resolveDebugAdapter(LaunchConfiguration *config, QString *error)
{
    if (!config) {
        if (error) {
            *error = QStringLiteral("Missing launch configuration");
        }
        return false;
    }
    if (!config->adapterCommand.trimmed().isEmpty()) {
        if (config->adapterArgs.isEmpty() && isGdbType(config->type)) {
            config->adapterArgs = QStringList{QStringLiteral("--interpreter=dap")};
        }
        return true;
    }

    if (isGdbType(config->type) || config->type.isEmpty()) {
        const QString gdb = config->arguments.value(QStringLiteral("miDebuggerPath")).toString();
        config->adapterCommand = gdb.isEmpty() ? QStringLiteral("gdb") : gdb;
        if (config->adapterArgs.isEmpty()) {
            config->adapterArgs = QStringList{QStringLiteral("--interpreter=dap")};
        }
        config->arguments.remove(QStringLiteral("miDebuggerPath"));
        return true;
    }
    if (config->type == QLatin1String("lldb")) {
        config->adapterCommand = QStringLiteral("lldb-dap");
        return true;
    }

    if (error) {
        *error = QStringLiteral("Unknown debug type '%1'. Use gdb, lldb, or adapterCommand.")
                     .arg(config->type);
    }
    return false;
}

QVector<LaunchConfiguration> parseLaunchJson(const QByteArray &json, QString *error)
{
    QVector<LaunchConfiguration> out;
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        if (error) {
            *error = parseError.errorString();
        }
        return out;
    }

    const QJsonArray configs = doc.object().value(QStringLiteral("configurations")).toArray();
    int index = 0;
    for (const QJsonValue &value : configs) {
        const QJsonObject obj = value.toObject();
        LaunchConfiguration config;
        config.name = obj.value(QStringLiteral("name")).toString().trimmed();
        config.type = obj.value(QStringLiteral("type")).toString().trimmed();
        config.request = obj.value(QStringLiteral("request")).toString().trimmed();
        if (config.request.isEmpty()) {
            config.request = QStringLiteral("launch");
        }
        config.adapterCommand = obj.value(QStringLiteral("adapterCommand")).toString().trimmed();
        config.adapterArgs = jsonStringList(obj.value(QStringLiteral("adapterArgs")));
        if (config.name.isEmpty()) {
            config.name = QStringLiteral("Launch %1").arg(index + 1);
        }

        QJsonObject arguments;
        for (auto it = obj.begin(); it != obj.end(); ++it) {
            const QString key = it.key();
            if (key == QLatin1String("name") || key == QLatin1String("type") ||
                key == QLatin1String("request") || key == QLatin1String("adapterCommand") ||
                key == QLatin1String("adapterArgs")) {
                continue;
            }
            arguments.insert(key, it.value());
        }
        config.arguments = arguments;
        out.append(config);
        ++index;
    }
    return out;
}

QVector<LaunchConfiguration> loadLaunchFile(const QString &workspaceRoot, QString *error)
{
    const QString path = launchFilePath(workspaceRoot);
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
    return parseLaunchJson(file.readAll(), error);
}

QByteArray defaultLaunchTemplateJson()
{
    return QByteArrayLiteral(R"({
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Launch",
            "type": "gdb",
            "request": "launch",
            "program": "${workspaceFolder}/build/debug/program.exe",
            "cwd": "${workspaceFolder}",
            "args": [],
            "stopOnEntry": false
        }
    ]
}
)");
}

bool writeLaunchTemplate(const QString &workspaceRoot, QString *error)
{
    if (workspaceRoot.isEmpty()) {
        if (error) {
            *error = QStringLiteral("No workspace folder");
        }
        return false;
    }
    QDir dir(workspaceRoot);
    if (!dir.mkpath(QStringLiteral(".editerako"))) {
        if (error) {
            *error = QStringLiteral("Could not create .editerako");
        }
        return false;
    }
    QFile file(launchFilePath(workspaceRoot));
    if (file.exists()) {
        return true;
    }
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (error) {
            *error = file.errorString();
        }
        return false;
    }
    if (file.write(defaultLaunchTemplateJson()) < 0) {
        if (error) {
            *error = file.errorString();
        }
        return false;
    }
    qCInfo(lcDap) << "Wrote launch template" << file.fileName();
    return true;
}
