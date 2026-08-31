#ifndef EDITERAKO_THEMEMANAGER_H
#define EDITERAKO_THEMEMANAGER_H

#include <QString>
#include <QStringList>

class QApplication;

class ThemeManager
{
public:
    static constexpr auto DarkId = "dark";
    static constexpr auto LightId = "light";

    [[nodiscard]] static QStringList availableThemeIds();
    static bool apply(QApplication &app, const QString &themeId);

private:
    [[nodiscard]] static QString resourcePathForTheme(const QString &themeId);
};

#endif
