#ifndef EDITERAKO_FILEEXPLORER_H
#define EDITERAKO_FILEEXPLORER_H

#include <QHash>
#include <QObject>
#include <QString>
#include <QStringList>

class QTreeWidget;
class QTreeWidgetItem;
class Workspace;

class FileExplorer : public QObject
{
    Q_OBJECT

public:
    FileExplorer(QTreeWidget *tree, Workspace *workspace, QObject *parent = nullptr);

    void reload();
    void collapseAll();
    [[nodiscard]] QString selectedDirectory() const;
    [[nodiscard]] QString selectedPath() const;
    void revealPath(const QString &path);
    void setPathBadges(const QHash<QString, QString> &badges);

signals:
    void fileActivated(const QString &path);
    void fileSelected(const QString &path);
    void directoryPopulated(const QString &path);
    void newFileRequested();
    void newFolderRequested();
    void openInTerminalRequested(const QString &directory);
    void treeMutated();

private:
    void populateChildren(QTreeWidgetItem *parent);
    void onItemExpanded(QTreeWidgetItem *item);
    void onItemClicked(QTreeWidgetItem *item, int column);
    void onItemDoubleClicked(QTreeWidgetItem *item, int column);
    void showContextMenu(const QPoint &pos);

    void collectExpandedPaths(QTreeWidgetItem *item, QStringList *out) const;
    [[nodiscard]] QTreeWidgetItem *findItemByPath(const QString &path) const;
    [[nodiscard]] QString itemPath(const QTreeWidgetItem *item) const;
    bool runOpThenReload(bool ok, const QString &revealPath = {});
    void applyBadge(QTreeWidgetItem *item) const;
    void applyBadgesRecursive(QTreeWidgetItem *item) const;
    void fitContentsColumn();

    QTreeWidget *m_tree = nullptr;
    Workspace *m_workspace = nullptr;
    QStringList m_clipPaths;
    bool m_clipCut = false;
    QHash<QString, QString> m_badges;
};

#endif
