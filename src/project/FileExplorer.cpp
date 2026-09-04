#include "project/FileExplorer.h"

#include "project/Workspace.h"
#include "project/WorkspaceOps.h"
#include "project/WorkspacePath.h"

#include <QApplication>
#include <QBrush>
#include <QClipboard>
#include <QColor>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QInputDialog>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QProcess>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QUrl>
#include <QtGlobal>
#include <algorithm>

namespace {

constexpr int kPathRole = Qt::UserRole;
constexpr int kDirRole = Qt::UserRole + 1;
constexpr int kLoadedRole = Qt::UserRole + 2;
constexpr int kNameRole = Qt::UserRole + 3;

QColor badgeColor(const QString &badge)
{
    if (badge == QLatin1String("M") || badge == QLatin1String("R") || badge == QLatin1String("C")) {
        return QColor(QStringLiteral("#d29922"));
    }
    if (badge == QLatin1String("A") || badge == QLatin1String("U")) {
        return QColor(QStringLiteral("#3fb950"));
    }
    if (badge == QLatin1String("D") || badge == QLatin1String("!")) {
        return QColor(QStringLiteral("#f85149"));
    }
    return QColor(QStringLiteral("#8b949e"));
}

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
    const QString icon = isDir ? QStringLiteral("📁") : iconForFile(name);
    if (badge.isEmpty()) {
        item->setText(0, QStringLiteral("%1 %2").arg(icon, name));
        item->setForeground(0, QBrush());
        return;
    }
    item->setText(0, QStringLiteral("%1 %2  %3").arg(icon, name, badge));
    item->setForeground(0, QBrush(badgeColor(badge)));
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

void FileExplorer::showContextMenu(const QPoint &pos)
{
    if (!m_tree || !m_workspace || !m_workspace->isValid()) {
        return;
    }

    QTreeWidgetItem *item = m_tree->itemAt(pos);
    if (item) {
        m_tree->setCurrentItem(item);
    }

    const QString path = selectedPath();
    const QString root = m_workspace->rootPath();
    const bool hasPath = !path.isEmpty() && isInsideWorkspace(root, path);
    const bool isRoot = hasPath && QDir::cleanPath(path) == QDir::cleanPath(root);
    const bool isDir = item && item->data(0, kDirRole).toBool();
    Q_UNUSED(isDir)

    QMenu menu(m_tree);
    QAction *newFile = menu.addAction(tr("New File"));
    QAction *newFolder = menu.addAction(tr("New Folder"));
    menu.addSeparator();
    QAction *rename = menu.addAction(tr("Rename"));
    QAction *del = menu.addAction(tr("Delete"));
    QAction *duplicate = menu.addAction(tr("Duplicate"));
    menu.addSeparator();
    QAction *copy = menu.addAction(tr("Copy"));
    QAction *cut = menu.addAction(tr("Cut"));
    QAction *paste = menu.addAction(tr("Paste"));
    menu.addSeparator();
    QAction *copyAbs = menu.addAction(tr("Copy Absolute Path"));
    QAction *copyRel = menu.addAction(tr("Copy Relative Path"));
    menu.addSeparator();
    QAction *revealOs = menu.addAction(tr("Reveal in File Manager"));
    QAction *openTerm = menu.addAction(tr("Open in Integrated Terminal"));
    menu.addSeparator();
    QAction *refresh = menu.addAction(tr("Refresh"));
    QAction *collapse = menu.addAction(tr("Collapse All"));

    rename->setEnabled(hasPath && !isRoot);
    del->setEnabled(hasPath && !isRoot);
    duplicate->setEnabled(hasPath && !isRoot);
    copy->setEnabled(hasPath && !isRoot);
    cut->setEnabled(hasPath && !isRoot);
    paste->setEnabled(!m_clipPaths.isEmpty());
    copyAbs->setEnabled(hasPath);
    copyRel->setEnabled(hasPath);
    revealOs->setEnabled(hasPath);
    openTerm->setEnabled(hasPath || !root.isEmpty());

    QAction *chosen = menu.exec(m_tree->viewport()->mapToGlobal(pos));
    if (!chosen) {
        return;
    }

    if (chosen == newFile) {
        emit newFileRequested();
        return;
    }
    if (chosen == newFolder) {
        emit newFolderRequested();
        return;
    }
    if (chosen == refresh) {
        reload();
        return;
    }
    if (chosen == collapse) {
        collapseAll();
        return;
    }
    if (chosen == openTerm) {
        emit openInTerminalRequested(selectedDirectory());
        return;
    }

    if (chosen == copyAbs && hasPath) {
        QApplication::clipboard()->setText(QDir::toNativeSeparators(path));
        return;
    }
    if (chosen == copyRel && hasPath) {
        QApplication::clipboard()->setText(QDir::fromNativeSeparators(QDir(root).relativeFilePath(path)));
        return;
    }
    if (chosen == revealOs && hasPath) {
#ifdef Q_OS_WIN
        QProcess::startDetached(QStringLiteral("explorer.exe"),
                                {QStringLiteral("/select,"), QDir::toNativeSeparators(path)});
#elif defined(Q_OS_MACOS)
        QProcess::startDetached(QStringLiteral("open"), {QStringLiteral("-R"), path});
#else
        QDesktopServices::openUrl(QUrl::fromLocalFile(isDir ? path : QFileInfo(path).absolutePath()));
#endif
        return;
    }
    if (chosen == copy && hasPath && !isRoot) {
        m_clipPaths = {path};
        m_clipCut = false;
        return;
    }
    if (chosen == cut && hasPath && !isRoot) {
        m_clipPaths = {path};
        m_clipCut = true;
        return;
    }
    if (chosen == paste) {
        const QString destDir = selectedDirectory();
        QString revealed;
        bool ok = true;
        for (const QString &src : m_clipPaths) {
            QString created;
            if (m_clipCut) {
                ok = moveInsideWorkspace(root, src, destDir, &created) && ok;
            } else {
                ok = copyInsideWorkspace(root, src, destDir, &created) && ok;
            }
            if (!created.isEmpty()) {
                revealed = created;
            }
        }
        if (m_clipCut && ok) {
            m_clipPaths.clear();
            m_clipCut = false;
        }
        runOpThenReload(ok, revealed);
        return;
    }
    if (chosen == duplicate && hasPath && !isRoot) {
        QString created;
        runOpThenReload(duplicateInsideWorkspace(root, path, &created), created);
        return;
    }
    if (chosen == rename && hasPath && !isRoot) {
        const QString currentName = QFileInfo(path).fileName();
        const QString name = QInputDialog::getText(m_tree, tr("Rename"), tr("New name:"),
                                                   QLineEdit::Normal, currentName);
        if (name.isEmpty() || name == currentName) {
            return;
        }
        QString created;
        if (!runOpThenReload(renameInsideWorkspace(root, path, name, &created), created)) {
            QMessageBox::warning(m_tree, tr("Rename"), tr("Could not rename \"%1\".").arg(currentName));
        }
        return;
    }
    if (chosen == del && hasPath && !isRoot) {
        const QString label = QFileInfo(path).fileName();
        const auto answer = QMessageBox::question(
            m_tree,
            tr("Delete"),
            tr("Move \"%1\" to the Recycle Bin?").arg(label),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        if (answer != QMessageBox::Yes) {
            return;
        }
        if (!runOpThenReload(deleteInsideWorkspace(root, path, WorkspaceDeleteMode::Trash))) {
            QMessageBox::warning(m_tree, tr("Delete"), tr("Could not move \"%1\" to the Recycle Bin.").arg(label));
        }
    }
}
