#include "plugins/PluginManager.h"

#include "core/CommandRegistry.h"
#include "core/Logging.h"
#include "plugins/IFileViewerProvider.h"
#include "plugins/PluginContext.h"
#include "syntax/LanguageRegistry.h"

#include <QAction>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLabel>
#include <QMenu>
#include <QPluginLoader>
#include <QStandardPaths>
#include <QVBoxLayout>
#include <QWidget>

namespace {

class ExtensionViewerProvider final : public IFileViewerProvider
{
public:
    ExtensionViewerProvider(QString id, QString title, QStringList extensions, QString pluginName)
        : m_id(std::move(id))
        , m_title(std::move(title))
        , m_extensions(std::move(extensions))
        , m_pluginName(std::move(pluginName))
    {
    }

    QString id() const override { return m_id; }

    bool canOpen(const QString &path) const override
    {
        const QString ext = QFileInfo(path).suffix().toLower();
        return m_extensions.contains(ext);
    }

    QWidget *create(const QString &path, QWidget *parent, QString *error) override
    {
        Q_UNUSED(error);
        auto *page = new QWidget(parent);
        auto *layout = new QVBoxLayout(page);
        auto *label = new QLabel(page);
        label->setWordWrap(true);
        label->setText(QStringLiteral("%1\n\n%2\n\n%3")
                           .arg(m_title,
                                QObject::tr("Contributed by plugin %1. A native library can replace "
                                            "this placeholder with a custom preview.")
                                    .arg(m_pluginName),
                                QDir::toNativeSeparators(path)));
        layout->addWidget(label);
        layout->addStretch(1);
        return page;
    }

private:
    QString m_id;
    QString m_title;
    QStringList m_extensions;
    QString m_pluginName;
};

QWidget *makePlaceholderPanel(const QString &title, const QString &pluginName, QWidget *parent)
{
    auto *page = new QWidget(parent);
    auto *layout = new QVBoxLayout(page);
    auto *label = new QLabel(page);
    label->setWordWrap(true);
    label->setText(QObject::tr("%1\n\nPlaceholder panel from plugin %2.")
                       .arg(title, pluginName));
    layout->addWidget(label);
    layout->addStretch(1);
    return page;
}

} // namespace

PluginManager::PluginManager(QObject *parent)
    : QObject(parent)
{
}

PluginManager::~PluginManager()
{
    blockSignals(true);
    unloadAll();
}

void PluginManager::setCommandRegistry(CommandRegistry *commands)
{
    m_commands = commands;
}

void PluginManager::setDialogParent(QWidget *parent)
{
    m_dialogParent = parent;
}

void PluginManager::setWorkspaceRoot(const QString &root)
{
    if (m_workspaceRoot == root) {
        return;
    }
    unloadSource(PluginInfo::Source::Workspace);
    m_workspaceRoot = root;
    loadDirectory(workspacePluginsDirectory(m_workspaceRoot), PluginInfo::Source::Workspace);
    emit pluginsChanged();
}

void PluginManager::setDisabledIds(const QStringList &ids)
{
    m_disabledIds = ids;
}

void PluginManager::setLogHandler(std::function<void(const QString &)> handler)
{
    m_log = std::move(handler);
}

void PluginManager::setViewerHandlers(std::function<void(IFileViewerProvider *)> add,
                                      std::function<void(IFileViewerProvider *)> remove)
{
    m_addViewer = std::move(add);
    m_removeViewer = std::move(remove);
}

void PluginManager::setPanelHandlers(std::function<void(const QString &, const QString &, QWidget *)> add,
                                     std::function<void(const QString &)> remove)
{
    m_addPanel = std::move(add);
    m_removePanel = std::move(remove);
}

QString PluginManager::userPluginsDirectory()
{
    return QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation))
        .filePath(QStringLiteral("plugins"));
}

QString PluginManager::workspacePluginsDirectory(const QString &workspaceRoot)
{
    if (workspaceRoot.isEmpty()) {
        return {};
    }
    return QDir(workspaceRoot).filePath(QStringLiteral(".editerako/plugins"));
}

QVector<PluginInfo> PluginManager::plugins() const
{
    QVector<PluginInfo> out;
    out.reserve(static_cast<int>(m_loaded.size()));
    for (const auto &plugin : m_loaded) {
        out.append(plugin->info);
    }
    return out;
}

PluginManager::LoadedPlugin *PluginManager::findById(const QString &id)
{
    for (auto &plugin : m_loaded) {
        if (plugin->info.id == id) {
            return plugin.get();
        }
    }
    return nullptr;
}

bool PluginManager::isDisabled(const QString &id) const
{
    return m_disabledIds.contains(id);
}

void PluginManager::reload()
{
    unloadAll();
    loadDirectory(userPluginsDirectory(), PluginInfo::Source::User);
    loadDirectory(workspacePluginsDirectory(m_workspaceRoot), PluginInfo::Source::Workspace);
    emit pluginsChanged();
}

void PluginManager::unloadAll()
{
    while (!m_loaded.empty()) {
        unload(m_loaded.back().get());
        m_loaded.pop_back();
    }
    applyLanguages();
    emit pluginsChanged();
}

void PluginManager::unloadSource(PluginInfo::Source source)
{
    for (int i = static_cast<int>(m_loaded.size()) - 1; i >= 0; --i) {
        if (m_loaded[static_cast<size_t>(i)]->info.source != source) {
            continue;
        }
        unload(m_loaded[static_cast<size_t>(i)].get());
        m_loaded.erase(m_loaded.begin() + i);
    }
    applyLanguages();
}

int PluginManager::loadDirectory(const QString &directory, PluginInfo::Source source)
{
    if (directory.isEmpty()) {
        return 0;
    }
    QDir dir(directory);
    if (!dir.exists()) {
        return 0;
    }

    int loaded = 0;
    const QFileInfoList entries = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo &entry : entries) {
        if (loadOne(entry.absoluteFilePath(), source)) {
            ++loaded;
        }
    }
    applyLanguages();
    emit pluginsChanged();
    return loaded;
}

bool PluginManager::addInProcessPlugin(const PluginManifest &manifest, IPlugin *plugin)
{
    if (!plugin || !isValidPluginId(manifest.id) || findById(manifest.id)) {
        return false;
    }
    auto loaded = std::make_unique<LoadedPlugin>();
    loaded->manifest = manifest;
    loaded->info.id = manifest.id;
    loaded->info.name = manifest.name.isEmpty() ? manifest.id : manifest.name;
    loaded->info.version = manifest.version;
    loaded->info.source = PluginInfo::Source::InProcess;
    loaded->info.enabled = !isDisabled(manifest.id);
    loaded->instance = plugin;
    loaded->ownsInstance = true;
    if (!loaded->info.enabled) {
        m_loaded.push_back(std::move(loaded));
        emit pluginsChanged();
        return true;
    }
    if (!activateLoaded(loaded.get())) {
        return false;
    }
    m_loaded.push_back(std::move(loaded));
    applyLanguages();
    emit pluginsChanged();
    return true;
}

bool PluginManager::loadOne(const QString &pluginDir, PluginInfo::Source source)
{
    const QString manifestPath = QDir(pluginDir).filePath(QStringLiteral("plugin.json"));
    QFile file(manifestPath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    QString error;
    const PluginManifest manifest = parsePluginManifest(file.readAll(), &error);
    if (manifest.id.isEmpty()) {
        qCWarning(lcPlugin) << "Skipping plugin" << pluginDir << error;
        return false;
    }
    if (findById(manifest.id)) {
        qCWarning(lcPlugin) << "Duplicate plugin id" << manifest.id;
        return false;
    }

    auto loaded = std::make_unique<LoadedPlugin>();
    loaded->manifest = manifest;
    loaded->info.id = manifest.id;
    loaded->info.name = manifest.name;
    loaded->info.version = manifest.version;
    loaded->info.directory = pluginDir;
    loaded->info.source = source;
    loaded->info.enabled = !isDisabled(manifest.id);
    if (!loaded->info.enabled) {
        m_loaded.push_back(std::move(loaded));
        return true;
    }
    if (!activateLoaded(loaded.get())) {
        return false;
    }
    m_loaded.push_back(std::move(loaded));
    return true;
}

bool PluginManager::activateLoaded(LoadedPlugin *loaded)
{
    if (!loaded) {
        return false;
    }

    PluginContext context(m_commands, m_dialogParent, m_workspaceRoot, loaded->info.directory);
    context.setLogHandler(m_log);
    context.setAddViewerHandler([this, loaded](IFileViewerProvider *provider) {
        if (!provider) {
            return;
        }
        loaded->viewers.append(provider);
        if (m_addViewer) {
            m_addViewer(provider);
        }
    });
    context.setAddPanelHandler([this, loaded](const QString &id, const QString &title, QWidget *widget) {
        loaded->panelIds.append(id);
        if (m_addPanel) {
            m_addPanel(id, title, widget);
        }
    });

    if (m_commands) {
        for (const PluginCommandContribution &command : loaded->manifest.commands) {
            if (m_commands->action(command.id)) {
                continue;
            }
            QAction *action = m_commands->create(command.id, command.title);
            if (!action) {
                continue;
            }
            loaded->commandIds.append(command.id);
            const QString pluginName = loaded->info.name;
            QObject::connect(action, &QAction::triggered, this, [this, command, pluginName]() {
                const QString text = tr("Plugin command %1 (%2)").arg(command.id, pluginName);
                if (m_log) {
                    m_log(text);
                }
                emit statusMessage(text, 2500);
            });
        }
    }

    for (const PluginViewerContribution &viewer : loaded->manifest.viewers) {
        auto *provider = new ExtensionViewerProvider(viewer.id, viewer.title, viewer.extensions,
                                                     loaded->info.name);
        loaded->viewers.append(provider);
        if (m_addViewer) {
            m_addViewer(provider);
        }
    }

    for (const PluginPanelContribution &panel : loaded->manifest.panels) {
        if (!m_addPanel) {
            continue;
        }
        QWidget *widget = makePlaceholderPanel(panel.title, loaded->info.name, m_dialogParent);
        loaded->panelIds.append(panel.id);
        m_addPanel(panel.id, panel.title, widget);
    }

    if (!loaded->manifest.library.isEmpty() && loaded->instance == nullptr) {
        QString libError;
        const QString libraryPath =
            resolvePluginLibraryPath(loaded->info.directory, loaded->manifest.library, &libError);
        if (libraryPath.isEmpty()) {
            loaded->info.error = libError;
            qCWarning(lcPlugin) << loaded->info.id << libError;
        } else if (!QFileInfo::exists(libraryPath)) {
            loaded->info.error = tr("Native library not found: %1").arg(libraryPath);
            qCWarning(lcPlugin) << loaded->info.error;
        } else {
            loaded->loader = new QPluginLoader(libraryPath);
            QObject *object = loaded->loader->instance();
            loaded->instance = qobject_cast<IPlugin *>(object);
            if (!loaded->instance) {
                loaded->info.error = loaded->loader->errorString();
                qCWarning(lcPlugin) << "Failed to load" << libraryPath << loaded->info.error;
                loaded->loader->unload();
                delete loaded->loader;
                loaded->loader = nullptr;
            }
        }
    }

    if (loaded->instance) {
        if (!loaded->instance->activate(context)) {
            loaded->info.error = tr("Plugin activate() failed");
            unload(loaded);
            return false;
        }
        loaded->info.nativeLoaded = loaded->loader != nullptr || loaded->info.source == PluginInfo::Source::InProcess;
    }

    qCInfo(lcPlugin) << "Loaded plugin" << loaded->info.id << loaded->info.version;
    return true;
}

void PluginManager::unload(LoadedPlugin *loaded)
{
    if (!loaded) {
        return;
    }
    if (loaded->instance) {
        loaded->instance->deactivate();
    }
    if (loaded->loader) {
        loaded->loader->unload();
        delete loaded->loader;
        loaded->loader = nullptr;
        loaded->instance = nullptr;
    } else if (loaded->ownsInstance) {
        delete loaded->instance;
        loaded->instance = nullptr;
    }
    if (m_commands) {
        for (const QString &id : loaded->commandIds) {
            m_commands->remove(id);
        }
    }
    if (m_removeViewer) {
        for (IFileViewerProvider *provider : loaded->viewers) {
            m_removeViewer(provider);
            delete provider;
        }
    } else {
        qDeleteAll(loaded->viewers);
    }
    loaded->viewers.clear();
    if (m_removePanel) {
        for (const QString &id : loaded->panelIds) {
            m_removePanel(id);
        }
    }
    loaded->panelIds.clear();
    loaded->commandIds.clear();
}

void PluginManager::applyLanguages() const
{
    QVector<LanguageRegistry::ExtraLanguage> extras;
    for (const auto &plugin : m_loaded) {
        if (!plugin->info.enabled) {
            continue;
        }
        for (const PluginLanguageContribution &language : plugin->manifest.languages) {
            LanguageRegistry::ExtraLanguage extra;
            extra.id = language.id;
            extra.displayName = language.displayName;
            extra.extensions = language.extensions;
            extras.append(extra);
        }
    }
    LanguageRegistry::setExtraLanguages(extras);
}

void PluginManager::fillMenu(QMenu *menu)
{
    if (!menu || !m_commands) {
        return;
    }
    menu->clear();
    bool any = false;
    for (const auto &plugin : m_loaded) {
        if (!plugin->info.enabled) {
            continue;
        }
        for (const QString &id : plugin->commandIds) {
            if (QAction *action = m_commands->action(id)) {
                menu->addAction(action);
                any = true;
            }
        }
    }
    if (!any) {
        auto *empty = menu->addAction(tr("No plugin commands"));
        empty->setEnabled(false);
    }
}
