#include "plugins/PluginContext.h"

#include "core/Logging.h"
#include "plugins/IFileViewerProvider.h"

PluginContext::PluginContext(CommandRegistry *commands, QWidget *dialogParent, QString workspaceRoot,
                             QString pluginDirectory)
    : m_commands(commands)
    , m_dialogParent(dialogParent)
    , m_workspaceRoot(std::move(workspaceRoot))
    , m_pluginDirectory(std::move(pluginDirectory))
{
}

void PluginContext::log(const QString &message) const
{
    if (m_log) {
        m_log(message);
        return;
    }
    qCInfo(lcPlugin) << message;
}

void PluginContext::addViewer(IFileViewerProvider *provider)
{
    if (m_addViewer && provider) {
        m_addViewer(provider);
    }
}

void PluginContext::addPanel(const QString &id, const QString &title, QWidget *widget)
{
    if (m_addPanel && widget && !id.isEmpty()) {
        m_addPanel(id, title, widget);
    }
}
