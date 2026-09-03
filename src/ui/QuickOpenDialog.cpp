#include "ui/QuickOpenDialog.h"

#include "core/FuzzyMatcher.h"
#include "project/WorkspaceFileIndex.h"

#include <QDir>
#include <QFileInfo>
#include <QSet>

namespace {

QString displayPath(const QString &root, const QString &absolute)
{
    if (root.isEmpty()) {
        return QDir::fromNativeSeparators(absolute);
    }
    const QString relative = QDir(root).relativeFilePath(absolute);
    if (relative.startsWith(QLatin1String(".."))) {
        return QDir::fromNativeSeparators(absolute);
    }
    return QDir::fromNativeSeparators(relative);
}

} // namespace

QuickOpenDialog::QuickOpenDialog(WorkspaceFileIndex *index,
                                 const QStringList &additionalPaths,
                                 QWidget *parent)
    : FuzzyPickerDialog(parent)
    , m_index(index)
    , m_additionalPaths(additionalPaths)
{
    setWindowTitle(tr("Quick Open"));
    setPlaceholderText(tr("Type a file name, optionally :line"));
    setObjectName(QStringLiteral("quickOpenDialog"));

    if (m_index) {
        connect(m_index, &WorkspaceFileIndex::indexUpdated, this, &QuickOpenDialog::reloadItems);
    }
    reloadItems();
}

QString QuickOpenDialog::selectedPath() const
{
    return selectedId();
}

int QuickOpenDialog::selectedLine() const
{
    return parseFileLineQuery(query()).line;
}

QString QuickOpenDialog::rankQuery() const
{
    return parseFileLineQuery(query()).pattern;
}

void QuickOpenDialog::reloadItems()
{
    QSet<QString> seen;
    QStringList paths;
    auto addPath = [&](const QString &path) {
        const QString normalized = QDir::cleanPath(path);
        if (normalized.isEmpty() || seen.contains(normalized)) {
            return;
        }
        seen.insert(normalized);
        paths.append(normalized);
    };

    for (const QString &path : m_additionalPaths) {
        addPath(path);
    }
    if (m_index) {
        for (const QString &path : m_index->files()) {
            addPath(path);
        }
    }

    const QString root = m_index ? m_index->rootPath() : QString();
    QList<FuzzyPickerItem> items;
    items.reserve(paths.size());
    for (const QString &path : paths) {
        FuzzyPickerItem item;
        item.id = path;
        item.display = displayPath(root, path);
        item.filterText = item.display + QLatin1Char(' ') + QFileInfo(path).fileName();
        items.append(item);
    }
    setItems(items);

    if (m_index && m_index->isIndexing()) {
        setStatusText(tr("Indexing workspace…"));
    } else if (items.isEmpty()) {
        setStatusText(tr("No files to open"));
    } else {
        setStatusText(tr("%1 files").arg(items.size()));
    }
}
