#ifndef EDITERAKO_FILEEXPLORER_H
#define EDITERAKO_FILEEXPLORER_H

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
    [[nodiscard]] QString selectedDirectory() const;
    void revealPath(const QString &path);

signals:
    void fileActivated(const QString &path);
    void fileSelected(const QString &path);
    void directoryPopulated(const QString &path);

private:
    void populateChildren(QTreeWidgetItem *parent);
    void onItemExpanded(QTreeWidgetItem *item);
    void onItemClicked(QTreeWidgetItem *item, int column);
    void onItemDoubleClicked(QTreeWidgetItem *item, int column);

    void collectExpandedPaths(QTreeWidgetItem *item, QStringList *out) const;
    [[nodiscard]] QTreeWidgetItem *findItemByPath(const QString &path) const;

    QTreeWidget *m_tree = nullptr;
    Workspace *m_workspace = nullptr;
};

#endif
