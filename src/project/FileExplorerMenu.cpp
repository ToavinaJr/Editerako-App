#include "project/FileExplorer.h"

#include "project/FileExplorerRoles.h"
#include "project/Workspace.h"
#include "project/WorkspaceOps.h"
#include "project/WorkspacePath.h"

#include <QApplication>
#include <QClipboard>
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

using FileExplorerRoles::kDirRole;

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
