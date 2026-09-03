#ifndef EDITERAKO_WORKSPACECONTROLLER_H
#define EDITERAKO_WORKSPACECONTROLLER_H

#include <QObject>
#include <QString>
#include <QStringList>

class FileExplorer;
class FileWatcher;
class QTreeWidget;
class Workspace;
class WorkspaceFileIndex;

class WorkspaceController : public QObject
{
    Q_OBJECT

public:
    WorkspaceController(QTreeWidget *tree, QObject *parent = nullptr);

    [[nodiscard]] Workspace *workspace() const { return m_workspace; }
    [[nodiscard]] FileExplorer *explorer() const { return m_explorer; }
    [[nodiscard]] WorkspaceFileIndex *fileIndex() const { return m_fileIndex; }

    void setRootPath(const QString &path);
    [[nodiscard]] QString rootPath() const;
    [[nodiscard]] QString targetDirectory() const;

    bool createEmptyFile(const QString &fileName, QString *absolutePath = nullptr);
    bool createDirectory(const QString &folderName, QString *absolutePath = nullptr);

    void syncWatchedFiles(const QStringList &openFilePaths);
    void ignoreNextChange(const QString &path);
    void refreshIfContains(const QString &path);
    void reloadExplorer();

signals:
    void rootPathChanged(const QString &path);
    void fileActivated(const QString &path);
    void fileSelected(const QString &path);
    void fileChangedOnDisk(const QString &path);

private:
    void rebuildFileIndex();

    Workspace *m_workspace = nullptr;
    FileExplorer *m_explorer = nullptr;
    FileWatcher *m_watcher = nullptr;
    WorkspaceFileIndex *m_fileIndex = nullptr;
};

#endif
