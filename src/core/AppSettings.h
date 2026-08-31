#ifndef EDITERAKO_APPSETTINGS_H
#define EDITERAKO_APPSETTINGS_H

#include <QSettings>
#include <QString>
#include <QStringList>

class AppSettings
{
public:
    AppSettings();

    [[nodiscard]] QString themeId() const;
    void setThemeId(const QString &themeId);

    [[nodiscard]] QString editorFontFamily() const;
    [[nodiscard]] int editorFontSize() const;
    [[nodiscard]] int editorTabSize() const;
    [[nodiscard]] bool editorWordWrap() const;
    [[nodiscard]] bool editorLineNumbers() const;

    [[nodiscard]] QStringList excludedFolders() const;

    [[nodiscard]] QString aiProvider() const;

    [[nodiscard]] qint64 largeFileWarnBytes() const;
    [[nodiscard]] qint64 largeFileDisableSyntaxBytes() const;

    static QStringList defaultExcludedFolders();

private:
    QSettings m_settings;
};

#endif
