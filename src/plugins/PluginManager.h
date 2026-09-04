#ifndef EDITERAKO_PLUGINMANAGER_H
#define EDITERAKO_PLUGINMANAGER_H

#include "plugins/IPlugin.h"
#include "plugins/PluginManifest.h"

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>
#include <functional>
#include <memory>

class CommandRegistry;
class IFileViewerProvider;
class QMenu;
class QPluginLoader;
class QWidget;

struct PluginInfo {
    enum class Source {
        User,
        Workspace,
        InProcess,
    };

    QString id;
    QString name;
    QString version;
    QString directory;
    QString error;
    Source source = Source::User;
    bool enabled = true;
    bool nativeLoaded = false;
};

class PluginManager : public QObject
{
    Q_OBJECT

public:
    explicit PluginManager(QObject *parent = nullptr);
    ~PluginManager() override;

    void setCommandRegistry(CommandRegistry *commands);
    void setDialogParent(QWidget *parent);
    void setWorkspaceRoot(const QString &root);
    void setDisabledIds(const QStringList &ids);
    [[nodiscard]] QStringList disabledIds() const { return m_disabledIds; }

    void setLogHandler(std::function<void(const QString &)> handler);
    void setViewerHandlers(std::function<void(IFileViewerProvider *)> add,
                           std::function<void(IFileViewerProvider *)> remove);
    void setPanelHandlers(std::function<void(const QString &, const QString &, QWidget *)> add,
                          std::function<void(const QString &)> remove);

    void reload();
    int loadDirectory(const QString &directory, PluginInfo::Source source);
    bool addInProcessPlugin(const PluginManifest &manifest, IPlugin *plugin);

    void unloadSource(PluginInfo::Source source);
    void unloadAll();

    void fillMenu(QMenu *menu);

    [[nodiscard]] QVector<PluginInfo> plugins() const;
    [[nodiscard]] static QString userPluginsDirectory();
    [[nodiscard]] static QString workspacePluginsDirectory(const QString &workspaceRoot);

signals:
    void pluginsChanged();
    void statusMessage(const QString &message, int timeoutMs);

private:
    struct LoadedPlugin {
        PluginInfo info;
        PluginManifest manifest;
        QPluginLoader *loader = nullptr;
        IPlugin *instance = nullptr;
        bool ownsInstance = false;
        QStringList commandIds;
        QVector<IFileViewerProvider *> viewers;
        QStringList panelIds;
    };

    bool loadOne(const QString &pluginDir, PluginInfo::Source source);
    bool activateLoaded(LoadedPlugin *loaded);
    void unload(LoadedPlugin *loaded);
    void applyLanguages() const;
    [[nodiscard]] bool isDisabled(const QString &id) const;
    [[nodiscard]] LoadedPlugin *findById(const QString &id);

    CommandRegistry *m_commands = nullptr;
    QWidget *m_dialogParent = nullptr;
    QString m_workspaceRoot;
    QStringList m_disabledIds;
    std::function<void(const QString &)> m_log;
    std::function<void(IFileViewerProvider *)> m_addViewer;
    std::function<void(IFileViewerProvider *)> m_removeViewer;
    std::function<void(const QString &, const QString &, QWidget *)> m_addPanel;
    std::function<void(const QString &)> m_removePanel;
    std::vector<std::unique_ptr<LoadedPlugin>> m_loaded;
};

#endif
