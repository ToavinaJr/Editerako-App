#ifndef EDITERAKO_PLUGINCONTEXT_H
#define EDITERAKO_PLUGINCONTEXT_H

#include <QString>
#include <functional>

class CommandRegistry;
class IFileViewerProvider;
class QWidget;

class PluginContext
{
public:
    PluginContext(CommandRegistry *commands, QWidget *dialogParent, QString workspaceRoot,
                  QString pluginDirectory);

    [[nodiscard]] CommandRegistry *commands() const { return m_commands; }
    [[nodiscard]] QWidget *dialogParent() const { return m_dialogParent; }
    [[nodiscard]] QString workspaceRoot() const { return m_workspaceRoot; }
    [[nodiscard]] QString pluginDirectory() const { return m_pluginDirectory; }

    void log(const QString &message) const;
    void addViewer(IFileViewerProvider *provider);
    void addPanel(const QString &id, const QString &title, QWidget *widget);

    void setLogHandler(std::function<void(const QString &)> handler) { m_log = std::move(handler); }
    void setAddViewerHandler(std::function<void(IFileViewerProvider *)> handler)
    {
        m_addViewer = std::move(handler);
    }
    void setAddPanelHandler(std::function<void(const QString &, const QString &, QWidget *)> handler)
    {
        m_addPanel = std::move(handler);
    }

private:
    CommandRegistry *m_commands = nullptr;
    QWidget *m_dialogParent = nullptr;
    QString m_workspaceRoot;
    QString m_pluginDirectory;
    std::function<void(const QString &)> m_log;
    std::function<void(IFileViewerProvider *)> m_addViewer;
    std::function<void(const QString &, const QString &, QWidget *)> m_addPanel;
};

#endif
