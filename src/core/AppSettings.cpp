#include "core/AppSettings.h"

#include <QVariant>

namespace {

constexpr auto kTheme = "theme";
constexpr auto kEditorFontFamily = "editor/fontFamily";
constexpr auto kEditorFontSize = "editor/fontSize";
constexpr auto kEditorTabSize = "editor/tabSize";
constexpr auto kEditorWordWrap = "editor/wordWrap";
constexpr auto kEditorLineNumbers = "editor/lineNumbers";
constexpr auto kExcludedFolders = "workspace/excludedFolders";
constexpr auto kAiProvider = "ai/provider";
constexpr auto kLargeFileWarnBytes = "files/largeFileWarnBytes";
constexpr auto kLargeFileDisableSyntaxBytes = "files/largeFileDisableSyntaxBytes";

constexpr auto kDefaultTheme = "dark";
constexpr auto kDefaultFontFamily = "Consolas";
constexpr int kDefaultFontSize = 13;
constexpr int kDefaultTabSize = 4;
constexpr qint64 kDefaultWarnBytes = 5LL * 1024 * 1024;
constexpr qint64 kDefaultDisableSyntaxBytes = 20LL * 1024 * 1024;

} // namespace

AppSettings::AppSettings()
    : m_settings()
{
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

QString AppSettings::themeId() const
{
    return m_settings.value(kTheme, kDefaultTheme).toString();
}

void AppSettings::setThemeId(const QString &themeId)
{
    m_settings.setValue(kTheme, themeId);
}

QString AppSettings::editorFontFamily() const
{
    return m_settings.value(kEditorFontFamily, kDefaultFontFamily).toString();
}

int AppSettings::editorFontSize() const
{
    return m_settings.value(kEditorFontSize, kDefaultFontSize).toInt();
}

int AppSettings::editorTabSize() const
{
    return m_settings.value(kEditorTabSize, kDefaultTabSize).toInt();
}

bool AppSettings::editorWordWrap() const
{
    return m_settings.value(kEditorWordWrap, false).toBool();
}

bool AppSettings::editorLineNumbers() const
{
    return m_settings.value(kEditorLineNumbers, true).toBool();
}

QStringList AppSettings::excludedFolders() const
{
    return m_settings.value(kExcludedFolders, defaultExcludedFolders()).toStringList();
}

QString AppSettings::aiProvider() const
{
    return m_settings.value(kAiProvider, QStringLiteral("gemini")).toString();
}

qint64 AppSettings::largeFileWarnBytes() const
{
    return m_settings.value(kLargeFileWarnBytes, QVariant::fromValue(kDefaultWarnBytes)).toLongLong();
}

qint64 AppSettings::largeFileDisableSyntaxBytes() const
{
    return m_settings.value(kLargeFileDisableSyntaxBytes, QVariant::fromValue(kDefaultDisableSyntaxBytes)).toLongLong();
}
