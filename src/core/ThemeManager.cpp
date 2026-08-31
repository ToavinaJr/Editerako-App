#include "core/ThemeManager.h"
#include "core/Logging.h"

#include <QApplication>
#include <QFile>

QStringList ThemeManager::availableThemeIds()
{
    return {QString::fromLatin1(DarkId), QString::fromLatin1(LightId)};
}

QString ThemeManager::resourcePathForTheme(const QString &themeId)
{
    const QString id = availableThemeIds().contains(themeId) ? themeId : QString::fromLatin1(DarkId);
    return QStringLiteral(":/editerako/themes/%1.qss").arg(id);
}

bool ThemeManager::apply(QApplication &app, const QString &themeId)
{
    const QString path = resourcePathForTheme(themeId);
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCWarning(lcCore) << "Unable to open theme" << path;
        return false;
    }

    const QString qss = QString::fromUtf8(file.readAll());
    app.setStyleSheet(qss);
    qCInfo(lcCore) << "Applied theme" << themeId << "from" << path;
    return true;
}
