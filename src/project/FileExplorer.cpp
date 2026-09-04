#include "project/FileExplorer.h"

#include "project/FileExplorerDecorations.h"
#include "project/FileExplorerRoles.h"
#include "project/Workspace.h"

#include <QBrush>
#include <QDir>
#include <QFileInfo>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QtGlobal>
#include <algorithm>

namespace {

using FileExplorerRoles::kDirRole;
using FileExplorerRoles::kLoadedRole;
using FileExplorerRoles::kNameRole;
using FileExplorerRoles::kPathRole;

int pathDepth(const QString &path)
{
    const QString cleaned = QDir::fromNativeSeparators(path);
    return static_cast<int>(cleaned.count(QLatin1Char('/')));
}

} // namespace

FileExplorer::FileExplorer(QTreeWidget *tree, Workspace *workspace, QObject *parent)
    : QObject(parent)
    , m_tree(tree)
    , m_workspace(workspace)
{
    Q_ASSERT(tree);
    Q_ASSERT(workspace);

    m_tree->setHeaderHidden(true);
    m_tree->setRootIsDecorated(true);
    m_tree->setAlternatingRowColors(false);
    m_tree->setUniformRowHeights(true);
    m_tree->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(m_tree, &QTreeWidget::itemExpanded, this, &FileExplorer::onItemExpanded);
    connect(m_tree, &QTreeWidget::itemClicked, this, &FileExplorer::onItemClicked);
    connect(m_tree, &QTreeWidget::itemDoubleClicked, this, &FileExplorer::onItemDoubleClicked);
    connect(m_tree, &QTreeWidget::customContextMenuRequested, this, &FileExplorer::showContextMenu);
}

void FileExplorer::populateChildren(QTreeWidgetItem *parent)
{
    if (!parent || !m_workspace) {
        return;
    }

    const QString path = parent->data(0, kPathRole).toString();
    const QList<Workspace::Entry> entries = m_workspace->listEntries(path);
    for (const Workspace::Entry &entry : entries) {
        auto *item = new QTreeWidgetItem(parent);
        item->setData(0, kPathRole, entry.absolutePath);
        item->setData(0, kDirRole, entry.isDirectory);
        item->setData(0, kNameRole, entry.name);
        if (entry.isDirectory) {
            item->setData(0, kLoadedRole, false);
            item->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
        } else {
            item->setData(0, kLoadedRole, true);
        }
        applyBadge(item);
    }
}

void FileExplorer::reload()
{
    QStringList expanded;
    for (int i = 0; i < m_tree->topLevelItemCount(); ++i) {
        collectExpandedPaths(m_tree->topLevelItem(i), &expanded);
    }

    m_tree->clear();
    if (!m_workspace || !m_workspace->isValid()) {
        return;
    }

    const QString root = m_workspace->rootPath();
    auto *rootItem = new QTreeWidgetItem(m_tree);
    rootItem->setData(0, kPathRole, root);
    rootItem->setData(0, kDirRole, true);
    rootItem->setData(0, kNameRole, QDir(root).dirName());
    applyBadge(rootItem);

    populateChildren(rootItem);
    rootItem->setData(0, kLoadedRole, true);
    rootItem->setExpanded(true);
    emit directoryPopulated(root);

    std::sort(expanded.begin(), expanded.end(), [](const QString &a, const QString &b) {
        return pathDepth(a) < pathDepth(b);
    });

    for (const QString &path : expanded) {
        if (QDir::cleanPath(path) == QDir::cleanPath(root)) {
            continue;
        }
        if (auto *item = findItemByPath(path)) {
            item->setExpanded(true);
        }
    }
}

void FileExplorer::onItemExpanded(QTreeWidgetItem *item)
{
    if (!item || item->data(0, kLoadedRole).toBool()) {
        return;
    }
    if (!item->data(0, kDirRole).toBool()) {
        return;
    }

    populateChildren(item);
    item->setData(0, kLoadedRole, true);
    emit directoryPopulated(item->data(0, kPathRole).toString());
    if (item->childCount() == 0) {
        item->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
    }
}

QString FileExplorer::selectedDirectory() const
{
    const QString fallback = m_workspace ? m_workspace->rootPath() : QString();
    if (!m_tree) {
        return fallback;
    }

    QTreeWidgetItem *item = m_tree->currentItem();
    if (!item) {
        return fallback;
    }

    const QString path = item->data(0, kPathRole).toString();
    if (path.isEmpty()) {
        return fallback;
    }
    if (item->data(0, kDirRole).toBool()) {
        return path;
    }
    return QFileInfo(path).absolutePath();
}

QString FileExplorer::selectedPath() const
{
    if (!m_tree || !m_tree->currentItem()) {
        return {};
    }
    return m_tree->currentItem()->data(0, kPathRole).toString();
}

void FileExplorer::collapseAll()
{
    if (!m_tree) {
        return;
    }
    m_tree->collapseAll();
    if (m_tree->topLevelItemCount() > 0) {
        m_tree->topLevelItem(0)->setExpanded(true);
    }
}

void FileExplorer::revealPath(const QString &path)
{
    if (!m_tree || !m_workspace || path.isEmpty()) {
        return;
    }

    const QString root = QDir::cleanPath(m_workspace->rootPath());
    const QString abs = QDir::cleanPath(QFileInfo(path).absoluteFilePath());
    if (root.isEmpty()) {
        return;
    }

    const QString relative = QDir::fromNativeSeparators(QDir(root).relativeFilePath(abs));
    if (relative.startsWith(QLatin1String(".."))) {
        return;
    }

    QString accum = root;
    const QStringList parts = relative.split(QLatin1Char('/'), Qt::SkipEmptyParts);
    for (int i = 0; i < parts.size(); ++i) {
        if (auto *item = findItemByPath(accum)) {
            item->setExpanded(true);
        }
        accum = QDir(accum).filePath(parts.at(i));
    }

    if (auto *item = findItemByPath(abs)) {
        m_tree->setCurrentItem(item);
        m_tree->scrollToItem(item);
    }
}

void FileExplorer::onItemClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column)
    if (!item) {
        return;
    }

    const QString path = item->data(0, kPathRole).toString();
    if (item->data(0, kDirRole).toBool()) {
        return;
    }
    emit fileSelected(path);
}

void FileExplorer::onItemDoubleClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column)
    if (!item) {
        return;
    }

    const QString path = item->data(0, kPathRole).toString();
    if (item->data(0, kDirRole).toBool()) {
        item->setExpanded(!item->isExpanded());
        return;
    }
    emit fileActivated(path);
}

void FileExplorer::collectExpandedPaths(QTreeWidgetItem *item, QStringList *out) const
{
    if (!item || !out) {
        return;
    }
    if (item->isExpanded()) {
        out->append(item->data(0, kPathRole).toString());
    }
    for (int i = 0; i < item->childCount(); ++i) {
        collectExpandedPaths(item->child(i), out);
    }
}

QTreeWidgetItem *FileExplorer::findItemByPath(const QString &path) const
{
    const QString normalized = QDir::cleanPath(path);

    const auto walk = [&](auto &&self, QTreeWidgetItem *item) -> QTreeWidgetItem * {
        if (!item) {
            return nullptr;
        }
        if (QDir::cleanPath(item->data(0, kPathRole).toString()) == normalized) {
            return item;
        }
        for (int i = 0; i < item->childCount(); ++i) {
            if (auto *found = self(self, item->child(i))) {
                return found;
            }
        }
        return nullptr;
    };

    for (int i = 0; i < m_tree->topLevelItemCount(); ++i) {
        if (auto *found = walk(walk, m_tree->topLevelItem(i))) {
            return found;
        }
    }
    return nullptr;
}

QString FileExplorer::itemPath(const QTreeWidgetItem *item) const
{
    return item ? item->data(0, kPathRole).toString() : QString();
}

void FileExplorer::setPathBadges(const QHash<QString, QString> &badges)
{
    m_badges.clear();
    for (auto it = badges.cbegin(); it != badges.cend(); ++it) {
        m_badges.insert(QDir::cleanPath(it.key()), it.value());
    }
    if (!m_tree) {
        return;
    }
    for (int i = 0; i < m_tree->topLevelItemCount(); ++i) {
        applyBadgesRecursive(m_tree->topLevelItem(i));
    }
}

void FileExplorer::applyBadge(QTreeWidgetItem *item) const
{
    if (!item) {
        return;
    }
    const QString name = item->data(0, kNameRole).toString();
    const bool isDir = item->data(0, kDirRole).toBool();
    const QString path = QDir::cleanPath(item->data(0, kPathRole).toString());
    const QString badge = m_badges.value(path);
    const QString icon = isDir ? QStringLiteral("📁") : fileExplorerFileIcon(name);
    item->setText(0, fileExplorerItemText(icon, name, badge));
    if (badge.isEmpty()) {
        item->setForeground(0, QBrush());
        return;
    }
    item->setForeground(0, QBrush(fileExplorerBadgeColor(badge)));
}

void FileExplorer::applyBadgesRecursive(QTreeWidgetItem *item) const
{
    applyBadge(item);
    if (!item) {
        return;
    }
    for (int i = 0; i < item->childCount(); ++i) {
        applyBadgesRecursive(item->child(i));
    }
}

bool FileExplorer::runOpThenReload(bool ok, const QString &reveal)
{
    if (!ok) {
        return false;
    }
    reload();
    if (!reveal.isEmpty()) {
        revealPath(reveal);
    }
    emit treeMutated();
    return true;
}
