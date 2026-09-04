#include "plugins/PluginManifest.h"

#include <QDir>
#include <QtTest>

class PluginManifestTest : public QObject
{
    Q_OBJECT

private slots:
    void parsesContributions();
    void rejectsBadIdAndApi();
    void rejectsEscapingLibrary();
};

void PluginManifestTest::parsesContributions()
{
    const QByteArray json = R"({
        "id": "example.hello",
        "name": "Hello",
        "version": "1.2.0",
        "apiVersion": 1,
        "contributes": {
            "commands": [{ "id": "hello.greet", "title": "Greet" }],
            "languages": [{ "id": "toml", "displayName": "TOML", "extensions": [".toml", "TOML"] }],
            "viewers": [{ "id": "hello.csv", "extensions": ["csv"] }],
            "aiProviders": [{ "id": "hello.ai", "title": "Hello AI" }],
            "panels": [{ "id": "hello.panel", "title": "Hello" }]
        }
    })";
    QString error;
    const PluginManifest manifest = parsePluginManifest(json, &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QCOMPARE(manifest.id, QStringLiteral("example.hello"));
    QCOMPARE(manifest.commands.size(), 1);
    QCOMPARE(manifest.languages.front().extensions, QStringList{QStringLiteral("toml")});
    QCOMPARE(manifest.viewers.size(), 1);
    QCOMPARE(manifest.aiProviders.front().id, QStringLiteral("hello.ai"));
    QCOMPARE(manifest.panels.front().title, QStringLiteral("Hello"));
}

void PluginManifestTest::rejectsBadIdAndApi()
{
    QString error;
    QVERIFY(parsePluginManifest(R"({"id":"../evil","name":"x"})", &error).id.isEmpty());
    QVERIFY(!error.isEmpty());
    error.clear();
    QVERIFY(parsePluginManifest(R"({"id":"ok.plugin","apiVersion":2})", &error).id.isEmpty());
    QVERIFY(error.contains(QStringLiteral("apiVersion")));
}

void PluginManifestTest::rejectsEscapingLibrary()
{
    QString error;
    QVERIFY(
        resolvePluginLibraryPath(QStringLiteral("C:/plugins/hello"), QStringLiteral("../x"), &error)
            .isEmpty());
    QVERIFY(!error.isEmpty());
    error.clear();
    QVERIFY(resolvePluginLibraryPath(QStringLiteral("C:/plugins/hello"),
                                     QStringLiteral("C:/Windows/system32/ntdll.dll"),
                                     &error)
                .isEmpty());
    QVERIFY(!error.isEmpty());
    error.clear();
    QVERIFY(resolvePluginLibraryPath(
                QStringLiteral("C:/plugins/hello"), QStringLiteral("/tmp/evil.so"), &error)
                .isEmpty());
    QVERIFY(!error.isEmpty());
}

QTEST_GUILESS_MAIN(PluginManifestTest)
#include "PluginManifestTest.moc"
