#include "core/TranslationLoader.h"

#include <QCoreApplication>
#include <QFile>
#include <QLocale>
#include <QtTest>

class TranslationLoaderTest : public QObject
{
    Q_OBJECT

private slots:
    void frenchAndEnglishLocales();
    void systemLocaleFallback();
    void catalogNames();
    void frenchTsContainsFindReplace();
    void installDoesNotCrash();
};

void TranslationLoaderTest::frenchAndEnglishLocales()
{
    QCOMPARE(TranslationLoader::localeForUiLanguage(QStringLiteral("fr")).language(), QLocale::French);
    QCOMPARE(TranslationLoader::localeForUiLanguage(QStringLiteral("FR")).language(), QLocale::French);
    QCOMPARE(TranslationLoader::localeForUiLanguage(QStringLiteral("fr_FR")).language(), QLocale::French);
    QCOMPARE(TranslationLoader::localeForUiLanguage(QStringLiteral("en")).language(), QLocale::English);
    QCOMPARE(TranslationLoader::localeForUiLanguage(QStringLiteral("en_US")).language(), QLocale::English);
}

void TranslationLoaderTest::systemLocaleFallback()
{
    QCOMPARE(TranslationLoader::localeForUiLanguage({}), QLocale::system());
    QCOMPARE(TranslationLoader::localeForUiLanguage(QStringLiteral("system")), QLocale::system());
}

void TranslationLoaderTest::catalogNames()
{
    QCOMPARE(TranslationLoader::catalogName(QLocale(QLocale::French, QLocale::France)),
             QStringLiteral("editerako_fr"));
    QCOMPARE(TranslationLoader::catalogName(QLocale(QLocale::English, QLocale::UnitedStates)),
             QStringLiteral("editerako_en"));
}

void TranslationLoaderTest::frenchTsContainsFindReplace()
{
    QFile file(QStringLiteral(EDITERAKO_TRANSLATIONS_DIR) + QStringLiteral("/editerako_fr.ts"));
    QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
    const QString xml = QString::fromUtf8(file.readAll());
    QVERIFY(xml.contains(QLatin1String("language=\"fr")));
    QVERIFY(xml.contains(QStringLiteral("<source>Find / Replace</source>")));
    QVERIFY(xml.contains(QStringLiteral("<translation>Rechercher / Remplacer</translation>")));
}

void TranslationLoaderTest::installDoesNotCrash()
{
    QVERIFY(QCoreApplication::instance());
    QVERIFY(TranslationLoader::install(QCoreApplication::instance(), QStringLiteral("en")) >= 0);
}

QTEST_GUILESS_MAIN(TranslationLoaderTest)
#include "TranslationLoaderTest.moc"
