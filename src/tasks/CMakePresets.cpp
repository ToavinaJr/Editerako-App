#include "tasks/CMakePresets.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

namespace {

QByteArray readFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    return file.readAll();
}

QString findTool(const QString &name)
{
    const QString found = QStandardPaths::findExecutable(name);
    if (!found.isEmpty()) {
        return found;
    }
#ifdef Q_OS_WIN
    return QStandardPaths::findExecutable(name + QStringLiteral(".exe"));
#else
    return {};
#endif
}

const CMakePresetInfo *findPreset(const QVector<CMakePresetInfo> &presets, const QString &name)
{
    for (const CMakePresetInfo &preset : presets) {
        if (preset.name == name) {
            return &preset;
        }
    }
    return nullptr;
}

} // namespace

QStringList CMakeWorkspace::visiblePresetNames() const
{
    QStringList names;
    for (const CMakePresetInfo &preset : configurePresets) {
        if (!preset.hidden && !preset.name.isEmpty()) {
            names.append(preset.name);
        }
    }
    return names;
}

QString CMakeWorkspace::binaryDirFor(const QString &presetName) const
{
    QString name = presetName;
    QString raw;
    QStringList seen;
    while (!name.isEmpty() && !seen.contains(name)) {
        seen.append(name);
        const CMakePresetInfo *preset = findPreset(configurePresets, name);
        if (!preset) {
            break;
        }
        if (!preset->binaryDir.isEmpty()) {
            raw = preset->binaryDir;
            break;
        }
        name = preset->inherits;
    }
    if (raw.isEmpty()) {
        raw = QStringLiteral("${sourceDir}/build/${presetName}");
        if (presetName.isEmpty()) {
            raw = QStringLiteral("${sourceDir}/build");
        }
    }
    return QDir::cleanPath(expandCMakeMacro(raw, sourceDir, presetName));
}

QString expandCMakeMacro(const QString &value, const QString &sourceDir, const QString &presetName)
{
    QString out = value;
    out.replace(QStringLiteral("${sourceDir}"), QDir::fromNativeSeparators(sourceDir));
    out.replace(QStringLiteral("${presetName}"), presetName);
    return out;
}

QVector<CMakePresetInfo> parseConfigurePresets(const QByteArray &json)
{
    QVector<CMakePresetInfo> out;
    const QJsonDocument doc = QJsonDocument::fromJson(json);
    if (!doc.isObject()) {
        return out;
    }
    const QJsonArray presets = doc.object().value(QStringLiteral("configurePresets")).toArray();
    for (const QJsonValue &value : presets) {
        const QJsonObject obj = value.toObject();
        CMakePresetInfo info;
        info.name = obj.value(QStringLiteral("name")).toString();
        if (info.name.isEmpty()) {
            continue;
        }
        info.displayName = obj.value(QStringLiteral("displayName")).toString();
        info.binaryDir = obj.value(QStringLiteral("binaryDir")).toString();
        info.hidden = obj.value(QStringLiteral("hidden")).toBool(false);
        const QJsonValue inherits = obj.value(QStringLiteral("inherits"));
        if (inherits.isString()) {
            info.inherits = inherits.toString();
        } else if (inherits.isArray() && !inherits.toArray().isEmpty()) {
            info.inherits = inherits.toArray().first().toString();
        }
        out.append(info);
    }
    return out;
}

CMakeWorkspace inspectCMakeWorkspace(const QString &workspaceRoot)
{
    CMakeWorkspace workspace;
    workspace.cmakeExecutable = findTool(QStringLiteral("cmake"));
    workspace.ctestExecutable = findTool(QStringLiteral("ctest"));
    if (workspaceRoot.isEmpty()) {
        return workspace;
    }

    const QDir root(workspaceRoot);
    const QString lists = root.filePath(QStringLiteral("CMakeLists.txt"));
    if (!QFileInfo::exists(lists)) {
        return workspace;
    }

    workspace.detected = true;
    workspace.sourceDir = QFileInfo(lists).absolutePath();
    workspace.configurePresets += parseConfigurePresets(
        readFile(root.filePath(QStringLiteral("CMakePresets.json"))));
    workspace.configurePresets += parseConfigurePresets(
        readFile(root.filePath(QStringLiteral("CMakeUserPresets.json"))));
    return workspace;
}
