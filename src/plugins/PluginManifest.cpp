#include "plugins/PluginManifest.h"

#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

namespace {

QStringList jsonStringList(const QJsonValue &value)
{
    QStringList out;
    const QJsonArray array = value.toArray();
    for (const QJsonValue &item : array) {
        const QString text = item.toString().trimmed();
        if (!text.isEmpty()) {
            out.append(text);
        }
    }
    return out;
}

QStringList normalizeExtensions(const QStringList &raw)
{
    QStringList out;
    for (QString ext : raw) {
        ext = ext.trimmed().toLower();
        if (ext.startsWith(QLatin1Char('.'))) {
            ext.remove(0, 1);
        }
        if (!ext.isEmpty() && !out.contains(ext)) {
            out.append(ext);
        }
    }
    return out;
}

bool isForbiddenPluginLibrary(const QString &library)
{
    const QString path = QDir::fromNativeSeparators(library.trimmed());
    if (path.isEmpty() || path.contains(QLatin1String(".."))) {
        return true;
    }
    if (QDir::isAbsolutePath(path) || QFileInfo(path).isAbsolute()) {
        return true;
    }
    if (path.size() >= 2 && path.at(0).isLetter() && path.at(1) == QLatin1Char(':')) {
        return true;
    }
    return path.startsWith(QLatin1String("//"));
}

} // namespace

bool isValidPluginId(const QString &id)
{
    static const QRegularExpression kId(QStringLiteral(R"(^[A-Za-z][A-Za-z0-9._-]*$)"));
    return kId.match(id).hasMatch();
}

PluginManifest parsePluginManifest(const QByteArray &json, QString *error)
{
    PluginManifest manifest;
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        if (error) {
            *error = parseError.error == QJsonParseError::NoError
                         ? QStringLiteral("plugin.json must be an object")
                         : parseError.errorString();
        }
        return {};
    }

    const QJsonObject obj = doc.object();
    manifest.id = obj.value(QStringLiteral("id")).toString().trimmed();
    manifest.name = obj.value(QStringLiteral("name")).toString().trimmed();
    manifest.version = obj.value(QStringLiteral("version")).toString().trimmed();
    manifest.apiVersion = obj.value(QStringLiteral("apiVersion")).toInt(1);
    manifest.library = obj.value(QStringLiteral("library")).toString().trimmed();
    if (manifest.name.isEmpty()) {
        manifest.name = manifest.id;
    }
    if (manifest.version.isEmpty()) {
        manifest.version = QStringLiteral("0.0.0");
    }
    if (!isValidPluginId(manifest.id)) {
        if (error) {
            *error = QStringLiteral("Invalid plugin id");
        }
        return {};
    }
    if (manifest.apiVersion != 1) {
        if (error) {
            *error = QStringLiteral("Unsupported apiVersion %1").arg(manifest.apiVersion);
        }
        return {};
    }

    const QJsonObject contributes = obj.value(QStringLiteral("contributes")).toObject();
    const QJsonArray commands = contributes.value(QStringLiteral("commands")).toArray();
    for (const QJsonValue &value : commands) {
        const QJsonObject item = value.toObject();
        PluginCommandContribution command;
        command.id = item.value(QStringLiteral("id")).toString().trimmed();
        command.title = item.value(QStringLiteral("title")).toString().trimmed();
        if (command.id.isEmpty()) {
            continue;
        }
        if (command.title.isEmpty()) {
            command.title = command.id;
        }
        manifest.commands.append(command);
    }

    const QJsonArray languages = contributes.value(QStringLiteral("languages")).toArray();
    for (const QJsonValue &value : languages) {
        const QJsonObject item = value.toObject();
        PluginLanguageContribution language;
        language.id = item.value(QStringLiteral("id")).toString().trimmed();
        language.displayName = item.value(QStringLiteral("displayName")).toString().trimmed();
        language.extensions =
            normalizeExtensions(jsonStringList(item.value(QStringLiteral("extensions"))));
        if (language.displayName.isEmpty()) {
            language.displayName = language.id;
        }
        if (language.extensions.isEmpty()) {
            continue;
        }
        manifest.languages.append(language);
    }

    const QJsonArray viewers = contributes.value(QStringLiteral("viewers")).toArray();
    for (const QJsonValue &value : viewers) {
        const QJsonObject item = value.toObject();
        PluginViewerContribution viewer;
        viewer.id = item.value(QStringLiteral("id")).toString().trimmed();
        viewer.title = item.value(QStringLiteral("title")).toString().trimmed();
        viewer.extensions =
            normalizeExtensions(jsonStringList(item.value(QStringLiteral("extensions"))));
        if (viewer.id.isEmpty() || viewer.extensions.isEmpty()) {
            continue;
        }
        if (viewer.title.isEmpty()) {
            viewer.title = viewer.id;
        }
        manifest.viewers.append(viewer);
    }

    const QJsonArray aiProviders = contributes.value(QStringLiteral("aiProviders")).toArray();
    for (const QJsonValue &value : aiProviders) {
        const QJsonObject item = value.toObject();
        PluginAiContribution provider;
        provider.id = item.value(QStringLiteral("id")).toString().trimmed();
        provider.title = item.value(QStringLiteral("title")).toString().trimmed();
        if (provider.id.isEmpty()) {
            continue;
        }
        if (provider.title.isEmpty()) {
            provider.title = provider.id;
        }
        manifest.aiProviders.append(provider);
    }

    const QJsonArray panels = contributes.value(QStringLiteral("panels")).toArray();
    for (const QJsonValue &value : panels) {
        const QJsonObject item = value.toObject();
        PluginPanelContribution panel;
        panel.id = item.value(QStringLiteral("id")).toString().trimmed();
        panel.title = item.value(QStringLiteral("title")).toString().trimmed();
        if (panel.id.isEmpty()) {
            continue;
        }
        if (panel.title.isEmpty()) {
            panel.title = panel.id;
        }
        manifest.panels.append(panel);
    }

    return manifest;
}

QString
resolvePluginLibraryPath(const QString &pluginDirectory, const QString &library, QString *error)
{
    if (isForbiddenPluginLibrary(library)) {
        if (error) {
            *error = QStringLiteral("Plugin library path is not allowed");
        }
        return {};
    }

    QString name = library;
#ifdef Q_OS_WIN
    if (!name.endsWith(QLatin1String(".dll"), Qt::CaseInsensitive)) {
        name += QStringLiteral(".dll");
    }
#elif defined(Q_OS_MACOS)
    if (!name.endsWith(QLatin1String(".dylib")) && !name.startsWith(QLatin1String("lib"))) {
        name += QStringLiteral(".dylib");
    }
#else
    if (!name.endsWith(QLatin1String(".so")) && !name.startsWith(QLatin1String("lib"))) {
        name = QStringLiteral("lib") + name + QStringLiteral(".so");
    }
#endif

    const QString root = QDir::cleanPath(QFileInfo(pluginDirectory).absoluteFilePath());
    const QString resolved = QDir::cleanPath(QDir(root).filePath(name));
    const QString prefix = root + QLatin1Char('/');
    if (resolved != root && !resolved.startsWith(prefix)) {
        if (error) {
            *error = QStringLiteral("Plugin library escapes plugin directory");
        }
        return {};
    }
    return resolved;
}
