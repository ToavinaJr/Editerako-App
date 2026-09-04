#include "core/AppSettings.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>

class AppSettingsTest : public QObject
{
    Q_OBJECT

private slots:
    void cleanup();
    void userSettingsRoundtrip();
    void workspaceOverlayWins();
    void hotExitDefaultsOn();
    void uiLanguageRoundtrip();
    void isolatedSettingsIgnoreUserProfile();
};

void AppSettingsTest::cleanup()
{
    AppSettings::setWorkspaceRoot({});
}

void AppSettingsTest::userSettingsRoundtrip()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QSettings store(dir.filePath(QStringLiteral("user.ini")), QSettings::IniFormat);
    AppSettings settings(store);
    settings.setEditorTabSize(8);
    settings.setEditorInsertSpaces(true);
    settings.setAutoSave(true);
    settings.setAiModel(QStringLiteral("custom-model"));
    settings.setTerminalUsePty(true);
    settings.setHotExit(false);

    AppSettings loaded(store);
    QCOMPARE(loaded.editorTabSize(), 8);
    QVERIFY(loaded.editorInsertSpaces());
    QVERIFY(loaded.autoSave());
    QCOMPARE(loaded.aiModel(), QStringLiteral("custom-model"));
    QVERIFY(loaded.terminalUsePty());
    QVERIFY(!loaded.hotExit());
}

void AppSettingsTest::workspaceOverlayWins()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QSettings store(dir.filePath(QStringLiteral("user.ini")), QSettings::IniFormat);
    AppSettings user(store);
    user.setEditorTabSize(8);

    QJsonObject editor{{QStringLiteral("tabSize"), 2}};
    QJsonObject terminal{{QStringLiteral("usePty"), true}};
    QJsonObject overlay{{QStringLiteral("editor"), editor}, {QStringLiteral("terminal"), terminal}};
    AppSettings layered(store, overlay);
    QCOMPARE(layered.editorTabSize(), 2);
    QCOMPARE(layered.editorFontSize(), 13);
    QVERIFY(layered.terminalUsePty());
}

void AppSettingsTest::hotExitDefaultsOn()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QSettings store(dir.filePath(QStringLiteral("user.ini")), QSettings::IniFormat);
    AppSettings settings(store);
    QVERIFY(settings.hotExit());
}

void AppSettingsTest::uiLanguageRoundtrip()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QSettings store(dir.filePath(QStringLiteral("user.ini")), QSettings::IniFormat);
    AppSettings settings(store);
    QVERIFY(settings.uiLanguage().isEmpty());
    settings.setUiLanguage(QStringLiteral("fr"));

    AppSettings loaded(store);
    QCOMPARE(loaded.uiLanguage(), QStringLiteral("fr"));
}

void AppSettingsTest::isolatedSettingsIgnoreUserProfile()
{
    QTemporaryDir workspace;
    QVERIFY(workspace.isValid());
    QVERIFY(QDir(workspace.path()).mkpath(QStringLiteral(".editerako")));
    QJsonObject editor{{QStringLiteral("tabSize"), 3}};
    QJsonObject root{{QStringLiteral("editor"), editor}};
    QFile file(AppSettings::workspaceSettingsFilePath(workspace.path()));
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(QJsonDocument(root).toJson());
    file.close();

    QTemporaryDir userDir;
    QSettings store(userDir.filePath(QStringLiteral("user.ini")), QSettings::IniFormat);
    AppSettings::setWorkspaceRoot(workspace.path());
    AppSettings settings(store);
    QCOMPARE(settings.editorTabSize(), 3);
}

QTEST_GUILESS_MAIN(AppSettingsTest)
#include "AppSettingsTest.moc"
