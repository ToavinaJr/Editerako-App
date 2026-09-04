#include "core/CommandRegistry.h"
#include "plugins/IPlugin.h"
#include "plugins/PluginContext.h"
#include "plugins/PluginManager.h"
#include "syntax/LanguageRegistry.h"
#include "viewers/FileKind.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QWidget>
#include <QtTest>

class PingPlugin : public IPlugin
{
public:
    QString id() const override { return QStringLiteral("test.ping"); }
    bool activate(PluginContext &context) override
    {
        activated = true;
        context.log(QStringLiteral("ping-activate"));
        return true;
    }
    void deactivate() override
    {
        if (deactivatedFlag) {
            *deactivatedFlag = true;
        }
    }

    bool activated = false;
    bool *deactivatedFlag = nullptr;
};

class PluginManagerTest : public QObject
{
    Q_OBJECT

private slots:
    void cleanup();
    void emptyDirectoryLoadsNothing();
    void loadsManifestPlugin();
    void disabledPluginDoesNotRegisterCommands();
    void inProcessPluginActivates();
};

void PluginManagerTest::cleanup()
{
    LanguageRegistry::setExtraLanguages({});
}

void PluginManagerTest::emptyDirectoryLoadsNothing()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    PluginManager manager;
    QCOMPARE(manager.loadDirectory(dir.path(), PluginInfo::Source::User), 0);
    QVERIFY(manager.plugins().isEmpty());
}

void PluginManagerTest::loadsManifestPlugin()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QVERIFY(QDir(dir.path()).mkpath(QStringLiteral("hello")));
    QFile file(dir.filePath(QStringLiteral("hello/plugin.json")));
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    file.write(R"({
        "id": "example.hello",
        "name": "Hello",
        "version": "0.1.0",
        "contributes": {
            "commands": [{ "id": "hello.greet", "title": "Greet" }],
            "languages": [{ "id": "toml", "displayName": "TOML", "extensions": [".toml"] }]
        }
    })");
    file.close();

    QWidget parent;
    CommandRegistry registry(&parent);
    PluginManager manager;
    manager.setCommandRegistry(&registry);
    QCOMPARE(manager.loadDirectory(dir.path(), PluginInfo::Source::User), 1);
    QCOMPARE(manager.plugins().size(), 1);
    QCOMPARE(manager.plugins().front().id, QStringLiteral("example.hello"));
    QVERIFY(registry.action(QStringLiteral("hello.greet")) != nullptr);
    QCOMPARE(LanguageRegistry::extraDisplayNameForPath(QStringLiteral("x.toml")),
             QStringLiteral("TOML"));
    QCOMPARE(fileKindForPath(QStringLiteral("x.toml")), FileKind::Text);
}

void PluginManagerTest::disabledPluginDoesNotRegisterCommands()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QVERIFY(QDir(dir.path()).mkpath(QStringLiteral("hello")));
    QFile file(dir.filePath(QStringLiteral("hello/plugin.json")));
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    file.write(R"({"id":"example.hello","contributes":{"commands":[{"id":"hello.greet"}]}})");
    file.close();

    QWidget parent;
    CommandRegistry registry(&parent);
    PluginManager manager;
    manager.setCommandRegistry(&registry);
    manager.setDisabledIds({QStringLiteral("example.hello")});
    QCOMPARE(manager.loadDirectory(dir.path(), PluginInfo::Source::User), 1);
    QVERIFY(!manager.plugins().front().enabled);
    QVERIFY(registry.action(QStringLiteral("hello.greet")) == nullptr);
}

void PluginManagerTest::inProcessPluginActivates()
{
    QWidget parent;
    CommandRegistry registry(&parent);
    PluginManager manager;
    manager.setCommandRegistry(&registry);
    PluginManifest manifest;
    manifest.id = QStringLiteral("test.ping");
    manifest.name = QStringLiteral("Ping");
    auto *plugin = new PingPlugin;
    bool deactivated = false;
    plugin->deactivatedFlag = &deactivated;
    QVERIFY(manager.addInProcessPlugin(manifest, plugin));
    QVERIFY(plugin->activated);
    manager.unloadAll();
    QVERIFY(deactivated);
}

QTEST_MAIN(PluginManagerTest)
#include "PluginManagerTest.moc"
