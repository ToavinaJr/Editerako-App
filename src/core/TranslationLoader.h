#ifndef EDITERAKO_TRANSLATIONLOADER_H
#define EDITERAKO_TRANSLATIONLOADER_H

#include <QLocale>
#include <QString>

class QCoreApplication;

class TranslationLoader
{
public:
    static constexpr auto ResourcePrefix = ":/i18n";

    [[nodiscard]] static QLocale localeForUiLanguage(const QString &uiLanguage);
    [[nodiscard]] static QString catalogName(const QLocale &locale);
    static int install(QCoreApplication *app, const QString &uiLanguage);
};

#endif
