#ifndef EDITERAKO_APPSETTINGS_H
#define EDITERAKO_APPSETTINGS_H

#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <memory>

class QSettings;

class AppSettings
{
public:
    AppSettings();
    explicit AppSettings(QSettings &userSettings);
    AppSettings(QSettings &userSettings, QJsonObject workspaceOverlay);
    ~AppSettings();

    AppSettings(const AppSettings &) = delete;
    AppSettings &operator=(const AppSettings &) = delete;

    static void setWorkspaceRoot(const QString &root);
    [[nodiscard]] static QString workspaceRoot();
    [[nodiscard]] static QString workspaceSettingsFilePath(const QString &root);

    [[nodiscard]] QString themeId() const;
    void setThemeId(const QString &themeId);

    [[nodiscard]] QString editorFontFamily() const;
    void setEditorFontFamily(const QString &family);
    [[nodiscard]] int editorFontSize() const;
    void setEditorFontSize(int pointSize);
    [[nodiscard]] int editorTabSize() const;
    void setEditorTabSize(int tabSize);
    [[nodiscard]] bool editorInsertSpaces() const;
    void setEditorInsertSpaces(bool insertSpaces);
    [[nodiscard]] bool editorWordWrap() const;
    void setEditorWordWrap(bool wrap);
    [[nodiscard]] bool editorLineNumbers() const;
    void setEditorLineNumbers(bool visible);

    [[nodiscard]] bool autoSave() const;
    void setAutoSave(bool enabled);
    [[nodiscard]] int autoSaveDelayMs() const;
    void setAutoSaveDelayMs(int delayMs);

    [[nodiscard]] QStringList excludedFolders() const;
    void setExcludedFolders(const QStringList &folders);

    [[nodiscard]] QString terminalShell() const;
    void setTerminalShell(const QString &shell);

    [[nodiscard]] QString aiProvider() const;
    void setAiProvider(const QString &provider);
    [[nodiscard]] QString aiModel() const;
    void setAiModel(const QString &model);
    [[nodiscard]] QString aiEndpoint() const;
    void setAiEndpoint(const QString &endpoint);

    [[nodiscard]] qint64 largeFileWarnBytes() const;
    void setLargeFileWarnBytes(qint64 bytes);
    [[nodiscard]] qint64 largeFileDisableSyntaxBytes() const;
    void setLargeFileDisableSyntaxBytes(qint64 bytes);

    static QStringList defaultExcludedFolders();
    static QString defaultAiModel();

private:
    [[nodiscard]] QVariant value(const char *key, const QVariant &fallback) const;
    void setValue(const char *key, const QVariant &value);

    std::unique_ptr<QSettings> m_ownedSettings;
    QSettings *m_userSettings = nullptr;
    QJsonObject m_workspaceOverlay;
};

#endif
