#include "project/WorkspaceController.h"

#include "project/FileExplorer.h"
#include "project/FileWatcher.h"
#include "project/Workspace.h"

WorkspaceController::WorkspaceController(QTreeWidget *tree, QObject *parent)
    : QObject(parent)
    , m_workspace(new Workspace(this))
    , m_explorer(new FileExplorer(tree, m_workspace, this))
    , m_watcher(new FileWatcher(this))
{
    connect(m_explorer, &FileExplorer::fileActivated, this, &WorkspaceController::fileActivated);
    connect(m_explorer, &FileExplorer::fileSelected, this, &WorkspaceController::fileSelected);
    connect(m_explorer, &FileExplorer::directoryPopulated, m_watcher, &FileWatcher::watchDirectory);
    connect(m_watcher, &FileWatcher::rootContentsChanged, m_explorer, &FileExplorer::reload);
    connect(m_watcher, &FileWatcher::fileChangedOnDisk, this, &WorkspaceController::fileChangedOnDisk);
}

void WorkspaceController::setRootPath(const QString &path)
{
    m_workspace->setRootPath(path);
    const QString root = m_workspace->rootPath();
    m_explorer->reload();
    m_watcher->setRootPath(root);
    emit rootPathChanged(root);
}

QString WorkspaceController::rootPath() const
{
    return m_workspace->rootPath();
}

QString WorkspaceController::targetDirectory() const
{
    return m_explorer->selectedDirectory();
}

bool WorkspaceController::createEmptyFile(const QString &fileName, QString *absolutePath)
{
    QString created;
    if (!Workspace::createEmptyFile(targetDirectory(), fileName, &created)) {
        return false;
    }
    m_explorer->reload();
    m_explorer->revealPath(created);
    if (absolutePath) {
        *absolutePath = created;
    }
    return true;
}

bool WorkspaceController::createDirectory(const QString &folderName, QString *absolutePath)
{
    QString created;
    if (!Workspace::createDirectory(targetDirectory(), folderName, &created)) {
        return false;
    }
    m_explorer->reload();
    m_explorer->revealPath(created);
    if (absolutePath) {
        *absolutePath = created;
    }
    return true;
}

void WorkspaceController::syncWatchedFiles(const QStringList &openFilePaths)
{
    m_watcher->setRootPath(rootPath());
    m_watcher->setFilePaths(openFilePaths);
}

void WorkspaceController::ignoreNextChange(const QString &path)
{
    m_watcher->ignoreNextChange(path);
}

void WorkspaceController::refreshIfContains(const QString &path)
{
    if (m_workspace->containsPath(path)) {
        m_explorer->reload();
    }
}

void WorkspaceController::reloadExplorer()
{
    m_explorer->reload();
}
