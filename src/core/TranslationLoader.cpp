#include "core/TranslationLoader.h"
#include "core/Logging.h"

#include <QCoreApplication>
#include <QLibraryInfo>
#include <QTranslator>

QLocale TranslationLoader::localeForUiLanguage(const QString &uiLanguage)
{
    const QString code = uiLanguage.trimmed().toLower();
    if (code.isEmpty() || code == QLatin1String("system")) {
        return QLocale::system();
    }
    if (code == QLatin1String("fr") || code.startsWith(QLatin1String("fr_"))) {
        return QLocale(QLocale::French, QLocale::France);
    }
    if (code == QLatin1String("en") || code.startsWith(QLatin1String("en_"))) {
        return QLocale(QLocale::English, QLocale::UnitedStates);
    }
    const QLocale parsed(code);
    if (parsed.language() != QLocale::C) {
        return parsed;
    }
    return QLocale::system();
}

QString TranslationLoader::catalogName(const QLocale &locale)
{
    if (locale.language() == QLocale::French) {
        return QStringLiteral("editerako_fr");
    }
    return QStringLiteral("editerako_en");
}

int TranslationLoader::install(QCoreApplication *app, const QString &uiLanguage)
{
    if (!app) {
        return 0;
    }

    const QLocale locale = localeForUiLanguage(uiLanguage);
    QLocale::setDefault(locale);

    int loaded = 0;
    auto *qtTranslator = new QTranslator(app);
    if (qtTranslator->load(locale, QStringLiteral("qtbase"), QStringLiteral("_"),
                           QLibraryInfo::path(QLibraryInfo::TranslationsPath))) {
        app->installTranslator(qtTranslator);
        ++loaded;
    } else {
        delete qtTranslator;
    }

    if (locale.language() == QLocale::English) {
        return loaded;
    }

    auto *appTranslator = new QTranslator(app);
    const QString prefix = QString::fromLatin1(ResourcePrefix);
    const bool ok = appTranslator->load(locale, QStringLiteral("editerako"), QStringLiteral("_"), prefix)
        || appTranslator->load(prefix + QLatin1Char('/') + catalogName(locale));
    if (ok) {
        app->installTranslator(appTranslator);
        ++loaded;
        qCInfo(lcCore) << "Loaded translation catalog" << catalogName(locale);
    } else {
        delete appTranslator;
        qCWarning(lcCore) << "No translation catalog for" << locale.name();
    }
    return loaded;
}
