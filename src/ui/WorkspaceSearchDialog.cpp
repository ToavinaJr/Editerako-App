#include "ui/WorkspaceSearchDialog.h"

#include "core/Logging.h"
#include "editor/CodeEditor.h"
#include "editor/EditorDocument.h"
#include "editor/EditorIo.h"
#include "editor/EditorManager.h"
#include "project/Workspace.h"
#include "project/WorkspaceController.h"
#include "project/WorkspacePath.h"

#include <QCheckBox>
#include <QDir>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHash>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSplitter>
#include <QTextCursor>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

namespace {

constexpr int kPathRole = Qt::UserRole;
constexpr int kHitRole = Qt::UserRole + 1;

} // namespace

WorkspaceSearchDialog::WorkspaceSearchDialog(WorkspaceController *workspace,
                                             EditorManager *editors,
                                             QWidget *parent)
    : QDialog(parent)
    , m_workspace(workspace)
    , m_editors(editors)
    , m_search(new WorkspaceSearch(this))
{
    setWindowTitle(tr("Search in Workspace"));
    setObjectName(QStringLiteral("workspaceSearchDialog"));
    setWindowModality(Qt::NonModal);
    resize(860, 560);

    auto *root = new QVBoxLayout(this);

    m_query = new QLineEdit(this);
    m_query->setPlaceholderText(tr("Search"));
    m_replace = new QLineEdit(this);
    m_replace->setPlaceholderText(tr("Replace"));
    m_include = new QLineEdit(this);
    m_include->setPlaceholderText(tr("*.cpp, *.h"));
    m_exclude = new QLineEdit(this);
    m_exclude->setPlaceholderText(tr("*.min.js"));

    auto *form = new QFormLayout;
    form->addRow(tr("Find"), m_query);
    form->addRow(tr("Replace"), m_replace);
    form->addRow(tr("Include"), m_include);
    form->addRow(tr("Exclude"), m_exclude);
    root->addLayout(form);

    auto *flags = new QHBoxLayout;
    m_caseSensitive = new QCheckBox(tr("Case sensitive"), this);
    m_wholeWord = new QCheckBox(tr("Whole word"), this);
    m_regex = new QCheckBox(tr("Regex"), this);
    flags->addWidget(m_caseSensitive);
    flags->addWidget(m_wholeWord);
    flags->addWidget(m_regex);
    flags->addStretch(1);
    root->addLayout(flags);

    auto *buttons = new QHBoxLayout;
    m_searchButton = new QPushButton(tr("Search"), this);
    m_cancelButton = new QPushButton(tr("Cancel"), this);
    m_replaceButton = new QPushButton(tr("Replace"), this);
    m_replaceAllButton = new QPushButton(tr("Replace All"), this);
    m_cancelButton->setEnabled(false);
    buttons->addWidget(m_searchButton);
    buttons->addWidget(m_cancelButton);
    buttons->addStretch(1);
    buttons->addWidget(m_replaceButton);
    buttons->addWidget(m_replaceAllButton);
    root->addLayout(buttons);

    auto *splitter = new QSplitter(Qt::Horizontal, this);
    m_tree = new QTreeWidget(this);
    m_tree->setObjectName(QStringLiteral("searchResultsTree"));
    m_tree->setHeaderHidden(true);
    m_tree->setUniformRowHeights(true);
    m_preview = new QPlainTextEdit(this);
    m_preview->setObjectName(QStringLiteral("searchPreview"));
    m_preview->setReadOnly(true);
    splitter->addWidget(m_tree);
    splitter->addWidget(m_preview);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);
    root->addWidget(splitter, 1);

    m_status = new QLabel(this);
    root->addWidget(m_status);

    connect(m_searchButton, &QPushButton::clicked, this, &WorkspaceSearchDialog::startSearch);
    connect(m_query, &QLineEdit::returnPressed, this, &WorkspaceSearchDialog::startSearch);
    connect(m_cancelButton, &QPushButton::clicked, m_search, &WorkspaceSearch::cancel);
    connect(m_replaceButton, &QPushButton::clicked, this, &WorkspaceSearchDialog::replaceCurrent);
    connect(m_replaceAllButton, &QPushButton::clicked, this, &WorkspaceSearchDialog::replaceAll);
    connect(m_tree, &QTreeWidget::itemSelectionChanged, this, &WorkspaceSearchDialog::onSelectionChanged);
    connect(m_tree, &QTreeWidget::itemActivated, this, [this](QTreeWidgetItem *) {
        openCurrent();
    });
    connect(m_search, &WorkspaceSearch::resultsReady, this, &WorkspaceSearchDialog::onResults);
    connect(m_search, &WorkspaceSearch::finished, this, &WorkspaceSearchDialog::onFinished);

    m_query->setFocus();
}

SearchOptions WorkspaceSearchDialog::currentOptions() const
{
    SearchOptions options;
    options.query = m_query->text();
    options.replacement = m_replace->text();
    options.regex = m_regex->isChecked();
    options.caseSensitive = m_caseSensitive->isChecked();
    options.wholeWord = m_wholeWord->isChecked();
    options.includeGlob = m_include->text();
    options.excludeGlob = m_exclude->text();
    options.useGitIgnore = true;
    return options;
}

QHash<QString, QString> WorkspaceSearchDialog::snapshotBuffers() const
{
    QHash<QString, QString> buffers;
    if (!m_editors) {
        return buffers;
    }
    for (const QString &path : m_editors->openFilePaths()) {
        if (CodeEditor *editor = m_editors->editorForPath(path)) {
            buffers.insert(QDir::cleanPath(path), editor->toPlainText());
        }
    }
    return buffers;
}

void WorkspaceSearchDialog::startSearch()
{
    if (!m_workspace || m_workspace->rootPath().isEmpty()) {
        m_status->setText(tr("Open a folder to search the workspace."));
        return;
    }
    const SearchOptions options = currentOptions();
    if (options.query.isEmpty()) {
        m_status->setText(tr("Enter a search query."));
        return;
    }
    const CompiledSearch compiled = compileSearch(options);
    if (!compiled.isValid()) {
        m_status->setText(tr("Invalid search: %1").arg(compiled.error));
        return;
    }

    m_hits.clear();
    m_tree->clear();
    m_preview->clear();
    m_searchButton->setEnabled(false);
    m_cancelButton->setEnabled(true);
    m_status->setText(tr("Searching…"));

    m_search->start(m_workspace->rootPath(),
                    m_workspace->workspace()->excludedNames(),
                    options,
                    snapshotBuffers());
}

void WorkspaceSearchDialog::onResults(const QList<SearchHit> &hits)
{
    m_hits.append(hits);
    rebuildTree();
}

void WorkspaceSearchDialog::onFinished(const SearchJobResult &result)
{
    m_searchButton->setEnabled(true);
    m_cancelButton->setEnabled(false);
    if (!result.error.isEmpty() && result.error != QLatin1String("empty")) {
        m_status->setText(tr("Search failed: %1").arg(result.error));
        return;
    }
    if (result.cancelled && m_hits.isEmpty()) {
        m_status->setText(tr("Search cancelled."));
        return;
    }
    m_status->setText(tr("%1 results in %2 files")
                          .arg(m_hits.size())
                          .arg(result.filesScanned));
}

void WorkspaceSearchDialog::rebuildTree()
{
    m_tree->clear();
    QHash<QString, QTreeWidgetItem *> files;
    for (int i = 0; i < m_hits.size(); ++i) {
        const SearchHit &hit = m_hits.at(i);
        QTreeWidgetItem *fileItem = files.value(hit.path);
        if (!fileItem) {
            fileItem = new QTreeWidgetItem(m_tree);
            const QString relative = m_workspace
                ? QDir::fromNativeSeparators(QDir(m_workspace->rootPath()).relativeFilePath(hit.path))
                : hit.path;
            fileItem->setText(0, relative);
            fileItem->setData(0, kPathRole, hit.path);
            fileItem->setData(0, kHitRole, -1);
            fileItem->setExpanded(true);
            files.insert(hit.path, fileItem);
        }
        auto *hitItem = new QTreeWidgetItem(fileItem);
        const QString snippet = hit.lineText.trimmed();
        hitItem->setText(0, tr("%1: %2").arg(hit.line).arg(snippet));
        hitItem->setData(0, kPathRole, hit.path);
        hitItem->setData(0, kHitRole, i);
    }
}

void WorkspaceSearchDialog::onSelectionChanged()
{
    const SearchHit hit = currentHit();
    if (hit.path.isEmpty()) {
        m_preview->clear();
        return;
    }
    const int from = qMax(1, hit.line - 3);
    const int to = hit.line + 3;
    QStringList lines = hit.lineText.isEmpty() ? QStringList() : QStringList{hit.lineText};
    Q_UNUSED(from)
    Q_UNUSED(to)
    m_preview->setPlainText(tr("%1\n%2").arg(hit.line).arg(hit.lineText));
    QTextCursor cursor = m_preview->textCursor();
    const int start = m_preview->toPlainText().indexOf(hit.lineText);
    if (start >= 0) {
        const int lineStart = m_preview->toPlainText().indexOf(QLatin1Char('\n')) + 1;
        cursor.setPosition(lineStart + hit.column);
        cursor.setPosition(lineStart + hit.column + hit.length, QTextCursor::KeepAnchor);
        m_preview->setTextCursor(cursor);
    }
}

SearchHit WorkspaceSearchDialog::currentHit() const
{
    QTreeWidgetItem *item = m_tree->currentItem();
    if (!item) {
        return {};
    }
    const int index = item->data(0, kHitRole).toInt();
    if (index < 0 || index >= m_hits.size()) {
        return {};
    }
    return m_hits.at(index);
}

void WorkspaceSearchDialog::openCurrent()
{
    SearchHit hit = currentHit();
    QTreeWidgetItem *item = m_tree->currentItem();
    if (hit.path.isEmpty() && item) {
        emit openHitRequested(item->data(0, kPathRole).toString(), 1, 0);
        return;
    }
    if (!hit.path.isEmpty()) {
        emit openHitRequested(hit.path, hit.line, hit.column);
    }
}

bool WorkspaceSearchDialog::applyReplacement(const QString &path, const QString &replacement, int *count)
{
    if (!m_workspace || !isInsideWorkspace(m_workspace->rootPath(), path)) {
        return false;
    }
    const CompiledSearch compiled = compileSearch(currentOptions());
    if (!compiled.isValid()) {
        return false;
    }

    if (m_editors) {
        if (CodeEditor *editor = m_editors->editorForPath(path)) {
            int n = 0;
            const QString updated = replaceInText(editor->toPlainText(), compiled, replacement, &n);
            if (n > 0) {
                QTextCursor cursor(editor->document());
                cursor.beginEditBlock();
                cursor.select(QTextCursor::Document);
                cursor.insertText(updated);
                cursor.endEditBlock();
            }
            if (count) {
                *count = n;
            }
            emit fileMutated(path);
            return n > 0;
        }
    }

    const TextLoadResult loaded = readTextFile(path);
    if (!loaded.ok) {
        return false;
    }
    int n = 0;
    const QString updated = replaceInText(loaded.text, compiled, replacement, &n);
    if (n <= 0) {
        if (count) {
            *count = 0;
        }
        return false;
    }
    const TextSaveResult saved = writeTextFile(path, updated, loaded.meta);
    if (count) {
        *count = saved.ok ? n : 0;
    }
    if (saved.ok) {
        emit fileMutated(path);
    }
    return saved.ok;
}

void WorkspaceSearchDialog::replaceCurrent()
{
    const SearchHit hit = currentHit();
    if (hit.path.isEmpty()) {
        return;
    }
    int count = 0;
    if (!applyReplacement(hit.path, m_replace->text(), &count)) {
        m_status->setText(tr("Replace failed."));
        return;
    }
    m_status->setText(tr("Replaced %1 occurrence(s) in %2").arg(count).arg(QFileInfo(hit.path).fileName()));
    startSearch();
}

void WorkspaceSearchDialog::replaceAll()
{
    if (m_hits.isEmpty()) {
        return;
    }
    QStringList files;
    for (const SearchHit &hit : m_hits) {
        if (!files.contains(hit.path)) {
            files.append(hit.path);
        }
    }
    int total = 0;
    int fileCount = 0;
    for (const QString &path : files) {
        int count = 0;
        if (applyReplacement(path, m_replace->text(), &count) && count > 0) {
            total += count;
            ++fileCount;
        }
    }
    m_status->setText(tr("Replaced %1 occurrence(s) in %2 files").arg(total).arg(fileCount));
    startSearch();
}
