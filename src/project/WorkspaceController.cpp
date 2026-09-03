#include "project/WorkspaceController.h"

#include "project/FileExplorer.h"
#include "project/FileWatcher.h"
#include "project/Workspace.h"
#include "project/WorkspaceFileIndex.h"
#include "project/WorkspacePath.h"

#include <QDir>
#include <QFile>

WorkspaceController::WorkspaceController(QTreeWidget *tree, QObject *parent)
    : QObject(parent)
    , m_workspace(new Workspace(this))
    , m_explorer(new FileExplorer(tree, m_workspace, this))
    , m_watcher(new FileWatcher(this))
    , m_fileIndex(new WorkspaceFileIndex(this))
{
    connect(m_explorer, &FileExplorer::fileActivated, this, &WorkspaceController::fileActivated);
    connect(m_explorer, &FileExplorer::fileSelected, this, &WorkspaceController::fileSelected);
    connect(m_explorer, &FileExplorer::directoryPopulated, m_watcher, &FileWatcher::watchDirectory);
    connect(m_explorer, &FileExplorer::treeMutated, this, &WorkspaceController::rebuildFileIndex);
    connect(m_watcher, &FileWatcher::rootContentsChanged, this, [this]() {
        m_explorer->reload();
        rebuildFileIndex();
    });
    connect(m_watcher, &FileWatcher::fileChangedOnDisk, this, &WorkspaceController::fileChangedOnDisk);
}

void WorkspaceController::setRootPath(const QString &path)
{
    m_workspace->setRootPath(path);
    const QString root = m_workspace->rootPath();
    m_explorer->reload();
    m_watcher->setRootPath(root);
    rebuildFileIndex();
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
    if (resolveInsideWorkspace(rootPath(), targetDirectory(), fileName).isEmpty()) {
        return false;
    }
    QString created;
    if (!Workspace::createEmptyFile(targetDirectory(), fileName, &created)) {
        return false;
    }
    if (!isInsideWorkspace(rootPath(), created)) {
        QFile::remove(created);
        return false;
    }
    m_explorer->reload();
    m_explorer->revealPath(created);
    rebuildFileIndex();
    if (absolutePath) {
        *absolutePath = created;
    }
    return true;
}

bool WorkspaceController::createDirectory(const QString &folderName, QString *absolutePath)
{
    if (resolveInsideWorkspace(rootPath(), targetDirectory(), folderName).isEmpty()) {
        return false;
    }
    QString created;
    if (!Workspace::createDirectory(targetDirectory(), folderName, &created)) {
        return false;
    }
    if (!isInsideWorkspace(rootPath(), created)) {
        QDir(created).removeRecursively();
        return false;
    }
    m_explorer->reload();
    m_explorer->revealPath(created);
    rebuildFileIndex();
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
    rebuildFileIndex();
}

void WorkspaceController::rebuildFileIndex()
{
    m_fileIndex->setRootPath(rootPath());
    m_fileIndex->setExcludedNames(m_workspace->excludedNames());
    m_fileIndex->rebuild();
}
