#include "core/AppSettings.h"

#include "core/Logging.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSettings>
#include <QVariant>
#include <QtGlobal>

namespace {

constexpr auto kTheme = "theme";
constexpr auto kUiLanguage = "ui/language";
constexpr auto kEditorFontFamily = "editor/fontFamily";
constexpr auto kEditorFontSize = "editor/fontSize";
constexpr auto kEditorTabSize = "editor/tabSize";
constexpr auto kEditorInsertSpaces = "editor/insertSpaces";
constexpr auto kEditorWordWrap = "editor/wordWrap";
constexpr auto kEditorLineNumbers = "editor/lineNumbers";
constexpr auto kEditorPreviewTabs = "editor/previewTabs";
constexpr auto kAutoSave = "files/autoSave";
constexpr auto kAutoSaveDelayMs = "files/autoSaveDelayMs";
constexpr auto kHotExit = "files/hotExit";
constexpr auto kExcludedFolders = "workspace/excludedFolders";
constexpr auto kTerminalShell = "terminal/shell";
constexpr auto kTerminalUsePty = "terminal/usePty";
constexpr auto kAiProvider = "ai/provider";
constexpr auto kAiModel = "ai/model";
constexpr auto kAiEndpoint = "ai/endpoint";
constexpr auto kDisabledPlugins = "plugins/disabled";
constexpr auto kLargeFileWarnBytes = "files/largeFileWarnBytes";
constexpr auto kLargeFileDisableSyntaxBytes = "files/largeFileDisableSyntaxBytes";

constexpr auto kDefaultTheme = "dark";
constexpr auto kDefaultFontFamily = "Consolas";
constexpr int kDefaultFontSize = 13;
constexpr int kDefaultTabSize = 4;
constexpr int kDefaultAutoSaveDelayMs = 1000;
constexpr qint64 kDefaultWarnBytes = 5LL * 1024 * 1024;
constexpr qint64 kDefaultDisableSyntaxBytes = 20LL * 1024 * 1024;

QString g_workspaceRoot;
QJsonObject g_workspaceOverlay;

QJsonValue jsonValueForKey(const QJsonObject &root, const QString &key)
{
    const QStringList parts = key.split(QLatin1Char('/'));
    QJsonValue current = root;
    for (const QString &part : parts) {
        if (!current.isObject()) {
            return {};
        }
        current = current.toObject().value(part);
    }
    return current;
}

QVariant variantFromJson(const QJsonValue &value)
{
    if (value.isUndefined() || value.isNull()) {
        return {};
    }
    if (value.isArray()) {
        QStringList list;
        const QJsonArray array = value.toArray();
        for (const QJsonValue &item : array) {
            list.append(item.toString());
        }
        return list;
    }
    if (value.isDouble()) {
        const double number = value.toDouble();
        const auto asInt = static_cast<qint64>(number);
        if (qFuzzyCompare(number, static_cast<double>(asInt))) {
            return QVariant::fromValue(asInt);
        }
        return number;
    }
    return value.toVariant();
}

QJsonObject loadWorkspaceOverlay(const QString &root)
{
    if (root.isEmpty()) {
        return {};
    }
    QFile file(AppSettings::workspaceSettingsFilePath(root));
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        qCWarning(lcCore) << "Invalid workspace settings" << file.fileName() << error.errorString();
        return {};
    }
    return document.object();
}

} // namespace

AppSettings::AppSettings()
    : m_ownedSettings(std::make_unique<QSettings>()), m_userSettings(m_ownedSettings.get()),
      m_workspaceOverlay(g_workspaceOverlay)
{}

AppSettings::AppSettings(QSettings &userSettings)
    : m_userSettings(&userSettings), m_workspaceOverlay(g_workspaceOverlay)
{}

AppSettings::AppSettings(QSettings &userSettings, QJsonObject workspaceOverlay)
    : m_userSettings(&userSettings), m_workspaceOverlay(std::move(workspaceOverlay))
{}

AppSettings::~AppSettings() = default;

void AppSettings::setWorkspaceRoot(const QString &root)
{
    const QString normalized =
        root.isEmpty() ? QString() : QDir::cleanPath(QFileInfo(root).absoluteFilePath());
    g_workspaceRoot = normalized;
    g_workspaceOverlay = loadWorkspaceOverlay(normalized);
}

QString AppSettings::workspaceRoot()
{
    return g_workspaceRoot;
}

QString AppSettings::workspaceSettingsFilePath(const QString &root)
{
    if (root.isEmpty()) {
        return {};
    }
    return QDir(root).filePath(QStringLiteral(".editerako/settings.json"));
}

QStringList AppSettings::defaultExcludedFolders()
{
    return {
        QStringLiteral(".git"),
        QStringLiteral("node_modules"),
        QStringLiteral("build"),
        QStringLiteral("dist"),
        QStringLiteral("out"),
        QStringLiteral("target"),
        QStringLiteral("venv"),
        QStringLiteral(".venv"),
        QStringLiteral(".idea"),
        QStringLiteral(".vs"),
    };
}

QString AppSettings::defaultAiModel()
{
    return {};
}

QVariant AppSettings::value(const char *key, const QVariant &fallback) const
{
    const QVariant overlay =
        variantFromJson(jsonValueForKey(m_workspaceOverlay, QLatin1String(key)));
    if (overlay.isValid()) {
        return overlay;
    }
    if (m_userSettings && m_userSettings->contains(QLatin1String(key))) {
        return m_userSettings->value(QLatin1String(key));
    }
    return fallback;
}

void AppSettings::setValue(const char *key, const QVariant &value)
{
    if (!m_userSettings) {
        return;
    }
    m_userSettings->setValue(QLatin1String(key), value);
    m_userSettings->sync();
}

QString AppSettings::themeId() const
{
    return value(kTheme, kDefaultTheme).toString();
}

void AppSettings::setThemeId(const QString &themeId)
{
    setValue(kTheme, themeId);
}

QString AppSettings::uiLanguage() const
{
    return value(kUiLanguage, QString()).toString().trimmed();
}

void AppSettings::setUiLanguage(const QString &language)
{
    setValue(kUiLanguage, language.trimmed());
}

QString AppSettings::editorFontFamily() const
{
    return value(kEditorFontFamily, kDefaultFontFamily).toString();
}

void AppSettings::setEditorFontFamily(const QString &family)
{
    setValue(kEditorFontFamily, family);
}

int AppSettings::editorFontSize() const
{
    return qBound(8, value(kEditorFontSize, kDefaultFontSize).toInt(), 72);
}

void AppSettings::setEditorFontSize(int pointSize)
{
    setValue(kEditorFontSize, qBound(8, pointSize, 72));
}

int AppSettings::editorTabSize() const
{
    return qBound(1, value(kEditorTabSize, kDefaultTabSize).toInt(), 16);
}

void AppSettings::setEditorTabSize(int tabSize)
{
    setValue(kEditorTabSize, qBound(1, tabSize, 16));
}

bool AppSettings::editorInsertSpaces() const
{
    return value(kEditorInsertSpaces, false).toBool();
}

void AppSettings::setEditorInsertSpaces(bool insertSpaces)
{
    setValue(kEditorInsertSpaces, insertSpaces);
}

bool AppSettings::editorWordWrap() const
{
    return value(kEditorWordWrap, false).toBool();
}

void AppSettings::setEditorWordWrap(bool wrap)
{
    setValue(kEditorWordWrap, wrap);
}

bool AppSettings::editorLineNumbers() const
{
    return value(kEditorLineNumbers, true).toBool();
}

void AppSettings::setEditorLineNumbers(bool visible)
{
    setValue(kEditorLineNumbers, visible);
}

bool AppSettings::editorPreviewTabs() const
{
    return value(kEditorPreviewTabs, true).toBool();
}

void AppSettings::setEditorPreviewTabs(bool enabled)
{
    setValue(kEditorPreviewTabs, enabled);
}

bool AppSettings::autoSave() const
{
    return value(kAutoSave, false).toBool();
}

void AppSettings::setAutoSave(bool enabled)
{
    setValue(kAutoSave, enabled);
}

int AppSettings::autoSaveDelayMs() const
{
    return qBound(500, value(kAutoSaveDelayMs, kDefaultAutoSaveDelayMs).toInt(), 60000);
}

void AppSettings::setAutoSaveDelayMs(int delayMs)
{
    setValue(kAutoSaveDelayMs, qBound(500, delayMs, 60000));
}

bool AppSettings::hotExit() const
{
    return value(kHotExit, true).toBool();
}

void AppSettings::setHotExit(bool enabled)
{
    setValue(kHotExit, enabled);
}

QStringList AppSettings::excludedFolders() const
{
    return value(kExcludedFolders, defaultExcludedFolders()).toStringList();
}

void AppSettings::setExcludedFolders(const QStringList &folders)
{
    setValue(kExcludedFolders, folders);
}

QString AppSettings::terminalShell() const
{
    return value(kTerminalShell, QString()).toString();
}

void AppSettings::setTerminalShell(const QString &shell)
{
    setValue(kTerminalShell, shell);
}

bool AppSettings::terminalUsePty() const
{
    return value(kTerminalUsePty, false).toBool();
}

void AppSettings::setTerminalUsePty(bool enabled)
{
    setValue(kTerminalUsePty, enabled);
}

QString AppSettings::aiProvider() const
{
    return value(kAiProvider, QStringLiteral("chatgpt")).toString();
}

void AppSettings::setAiProvider(const QString &provider)
{
    setValue(kAiProvider, provider);
}

QString AppSettings::aiModel() const
{
    return value(kAiModel, defaultAiModel()).toString();
}

void AppSettings::setAiModel(const QString &model)
{
    setValue(kAiModel, model);
}

QString AppSettings::aiEndpoint() const
{
    return value(kAiEndpoint, QString()).toString();
}

void AppSettings::setAiEndpoint(const QString &endpoint)
{
    setValue(kAiEndpoint, endpoint);
}

qint64 AppSettings::largeFileWarnBytes() const
{
    return qMax(qint64(1),
                value(kLargeFileWarnBytes, QVariant::fromValue(kDefaultWarnBytes)).toLongLong());
}

void AppSettings::setLargeFileWarnBytes(qint64 bytes)
{
    setValue(kLargeFileWarnBytes, QVariant::fromValue(qMax(qint64(1), bytes)));
}

qint64 AppSettings::largeFileDisableSyntaxBytes() const
{
    return qMax(qint64(1),
                value(kLargeFileDisableSyntaxBytes, QVariant::fromValue(kDefaultDisableSyntaxBytes))
                    .toLongLong());
}

void AppSettings::setLargeFileDisableSyntaxBytes(qint64 bytes)
{
    setValue(kLargeFileDisableSyntaxBytes, QVariant::fromValue(qMax(qint64(1), bytes)));
}

QStringList AppSettings::disabledPlugins() const
{
    return value(kDisabledPlugins, QStringList{}).toStringList();
}

void AppSettings::setDisabledPlugins(const QStringList &ids)
{
    setValue(kDisabledPlugins, ids);
}
