#include "project/FileExplorer.h"

#include "project/Workspace.h"

#include <QDir>
#include <QFileInfo>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <algorithm>

namespace {

constexpr int kPathRole = Qt::UserRole;
constexpr int kDirRole = Qt::UserRole + 1;
constexpr int kLoadedRole = Qt::UserRole + 2;

QString iconForFile(const QString &fileName)
{
    const QString ext = QFileInfo(fileName).suffix().toLower();
    if (ext == QLatin1String("cpp") || ext == QLatin1String("cxx") || ext == QLatin1String("cc")
        || ext == QLatin1String("c")) {
        return QStringLiteral("🔵");
    }
    if (ext == QLatin1String("h") || ext == QLatin1String("hpp") || ext == QLatin1String("hxx")) {
        return QStringLiteral("🟦");
    }
    if (ext == QLatin1String("py")) {
        return QStringLiteral("🐍");
    }
    if (ext == QLatin1String("js")) {
        return QStringLiteral("🟨");
    }
    if (ext == QLatin1String("html") || ext == QLatin1String("htm")) {
        return QStringLiteral("🌐");
    }
    if (ext == QLatin1String("css")) {
        return QStringLiteral("🎨");
    }
    if (ext == QLatin1String("php")) {
        return QStringLiteral("🐘");
    }
    if (ext == QLatin1String("txt")) {
        return QStringLiteral("📝");
    }
    if (ext == QLatin1String("json")) {
        return QStringLiteral("📋");
    }
    if (ext == QLatin1String("xml") || ext == QLatin1String("ui")) {
        return QStringLiteral("📄");
    }
    if (ext == QLatin1String("exe") || ext == QLatin1String("bin")) {
        return QStringLiteral("⚙️");
    }
    return QStringLiteral("📄");
}

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
        if (entry.isDirectory) {
            item->setText(0, QStringLiteral("📁 %1").arg(entry.name));
            item->setData(0, kLoadedRole, false);
            item->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
        } else {
            item->setText(0, QStringLiteral("%1 %2").arg(iconForFile(entry.name), entry.name));
            item->setData(0, kLoadedRole, true);
        }
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
    rootItem->setText(0, QStringLiteral("📁 %1").arg(QDir(root).dirName()));
    rootItem->setData(0, kPathRole, root);
    rootItem->setData(0, kDirRole, true);

    populateChildren(rootItem);
    rootItem->setData(0, kLoadedRole, true);
    rootItem->setExpanded(true);

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
