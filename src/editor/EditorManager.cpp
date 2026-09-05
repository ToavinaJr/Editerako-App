#include "editor/EditorManager.h"

#include "core/AppSettings.h"
#include "core/Logging.h"
#include "editor/CodeEditor.h"
#include "editor/EditorArea.h"
#include "editor/EditorDocument.h"
#include "editor/EditorGroup.h"
#include "editor/EditorIo.h"
#include "editor/EditorStyle.h"
#include "editor/HighlighterSync.h"
#include "editor/TabOps.h"

#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMenu>
#include <QMessageBox>
#include <QTabWidget>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QUrl>
#include <QtGlobal>

namespace {

constexpr auto kTabBaseLabel = "tabBaseLabel";

QString decoratedTabLabel(const QString &base, bool modified, bool preview, bool pinned)
{
    QString label = base;
    if (modified) {
        label += QLatin1Char('*');
    }
    if (preview) {
        label = QLatin1Char('(') + label + QLatin1Char(')');
    }
    if (pinned) {
        label.prepend(QStringLiteral("📌 "));
    }
    return label;
}

QList<TabCloseFlags> flagsForGroup(EditorGroup *group)
{
    QList<TabCloseFlags> flags;
    if (!group) {
        return flags;
    }
    QTabWidget *tabs = group->tabWidget();
    for (int i = 0; i < tabs->count(); ++i) {
        TabCloseFlags flag;
        flag.pinned = group->isPinned(i);
        if (auto *editor = qobject_cast<CodeEditor *>(tabs->widget(i))) {
            if (auto *doc = EditorDocument::fromEditor(editor)) {
                flag.modified = doc->isModified();
            }
        }
        flags.append(flag);
    }
    return flags;
}

} // namespace

EditorManager::EditorManager(QWidget *dialogParent)
    : QObject(dialogParent)
    , m_dialogParent(dialogParent)
    , m_area(new EditorArea(dialogParent))
{
    m_active = createGroup();
    m_area->setInitialGroup(m_active);
    m_active->setActive(true);

    if (qApp) {
        connect(qApp, &QApplication::focusChanged, this, [this](QWidget *, QWidget *now) {
            if (EditorGroup *group = groupForWidget(now)) {
                setActiveGroup(group);
            }
        });
    }

    openUntitled();
}

QWidget *EditorManager::containerWidget() const
{
    return m_area;
}

QTabWidget *EditorManager::tabWidget() const
{
    return m_active ? m_active->tabWidget() : nullptr;
}

int EditorManager::activeTabCount() const
{
    return tabWidget() ? tabWidget()->count() : 0;
}

int EditorManager::totalTabCount() const
{
    int count = 0;
    if (!m_area) {
        return 0;
    }
    for (EditorGroup *group : m_area->groups()) {
        count += group->tabWidget()->count();
    }
    return count;
}

int EditorManager::groupCount() const
{
    return m_area ? m_area->groupCount() : 0;
}

CodeEditor *EditorManager::currentEditor() const
{
    return tabWidget() ? qobject_cast<CodeEditor *>(tabWidget()->currentWidget()) : nullptr;
}

EditorDocument *EditorManager::currentDocument() const
{
    return EditorDocument::fromEditor(currentEditor());
}

QWidget *EditorManager::currentWidget() const
{
    return tabWidget() ? tabWidget()->currentWidget() : nullptr;
}

QString EditorManager::currentFilePath() const
{
    return pathForWidget(currentWidget());
}

void EditorManager::setWorkingDirectory(const QString &dir)
{
    m_workingDirectory = dir;
}

EditorGroup *EditorManager::createGroup()
{
    auto *group = new EditorGroup(m_area);
    connect(group, &EditorGroup::currentChanged, this, [this, group]() {
        if (m_active != group) {
            setActiveGroup(group);
        } else {
            emit currentChanged();
        }
    });
    connect(group, &EditorGroup::tabCloseRequested, this, [this, group](int index) {
        setActiveGroup(group);
        closeInGroup(group, index);
    });
    connect(group, &EditorGroup::tabContextMenuRequested, this,
            [this, group](int index, const QPoint &globalPos) {
                setActiveGroup(group);
                showTabContextMenu(group, index, globalPos);
            });
    return group;
}

void EditorManager::setActiveGroup(EditorGroup *group)
{
    if (!group || m_active == group) {
        return;
    }
    if (m_active) {
        m_active->setActive(false);
    }
    m_active = group;
    m_active->setActive(true);
    emit currentChanged();
}

CodeEditor *EditorManager::createEditor()
{
    auto *editor = new CodeEditor(tabWidget());
    EditorStyle::apply(editor);
    connect(editor, &CodeEditor::fontZoomRequested, this, &EditorManager::adjustFontSize);

    auto *doc = new EditorDocument(editor, this);
    bindDocument(editor, doc);
    return editor;
}

void EditorManager::bindDocument(CodeEditor *editor, EditorDocument *doc)
{
    Q_UNUSED(editor);
    connect(doc, &EditorDocument::modificationChanged, this, [this, doc](bool modified) {
        if (modified) {
            for (CodeEditor *editor : editors()) {
                if (EditorDocument::fromEditor(editor) != doc) {
                    continue;
                }
                if (EditorGroup *group = groupForWidget(editor)) {
                    const int index = group->tabWidget()->indexOf(editor);
                    if (group->isPreview(index)) {
                        group->promote(index);
                    }
                }
            }
        }
        updateTabLabelsFor(doc);
        emit modificationChanged();
    });
    connect(doc, &EditorDocument::filePathChanged, this, [this, doc](const QString &) {
        updateTabLabelsFor(doc);
        if (doc->editor()) {
            HighlighterSync::apply(doc->editor());
        }
    });
}

CodeEditor *EditorManager::cloneEditorView(CodeEditor *source)
{
    auto *editor = new CodeEditor(tabWidget());
    EditorStyle::apply(editor);
    connect(editor, &CodeEditor::fontZoomRequested, this, &EditorManager::adjustFontSize);
    editor->setDocument(source->document());
    editor->setReadOnly(source->isReadOnly());
    editor->setLineNumbersVisible(source->isLineNumbersVisible());
    HighlighterSync::apply(editor);
    return editor;
}

void EditorManager::updateTabLabel(CodeEditor *editor)
{
    if (!editor) {
        return;
    }
    if (auto *doc = EditorDocument::fromEditor(editor)) {
        editor->setProperty(kTabBaseLabel, doc->displayName());
    }
    refreshWidgetTab(editor);
}

void EditorManager::refreshWidgetTab(QWidget *widget)
{
    EditorGroup *group = groupForWidget(widget);
    if (!group) {
        return;
    }
    refreshTab(group, group->tabWidget()->indexOf(widget));
}

void EditorManager::refreshGroupTabs(EditorGroup *group)
{
    if (!group) {
        return;
    }
    for (int i = 0; i < group->tabWidget()->count(); ++i) {
        refreshTab(group, i);
    }
}

void EditorManager::refreshTab(EditorGroup *group, int index)
{
    if (!group || index < 0 || index >= group->tabWidget()->count()) {
        return;
    }
    QWidget *widget = group->tabWidget()->widget(index);
    if (!widget) {
        return;
    }

    QString base = widget->property(kTabBaseLabel).toString();
    bool modified = false;
    if (auto *editor = qobject_cast<CodeEditor *>(widget)) {
        auto *doc = EditorDocument::fromEditor(editor);
        if (base.isEmpty()) {
            base = doc ? doc->displayName() : tr("untitled");
        }
        modified = doc && doc->isModified();
    } else if (base.isEmpty()) {
        base = widget->windowTitle();
        if (base.isEmpty()) {
            base = QFileInfo(pathForWidget(widget)).fileName();
        }
    }

    const bool preview = group->isPreview(index);
    const bool pinned = group->isPinned(index);
    group->tabWidget()->setTabText(index, decoratedTabLabel(base, modified, preview, pinned));

    const QString path = pathForWidget(widget);
    if (preview && !path.isEmpty()) {
        group->tabWidget()->setTabToolTip(index, tr("Preview — %1").arg(path));
    } else {
        group->tabWidget()->setTabToolTip(index, path);
    }
}

void EditorManager::updateTabLabelsFor(EditorDocument *doc)
{
    if (!doc) {
        return;
    }
    for (CodeEditor *editor : editors()) {
        if (EditorDocument::fromEditor(editor) == doc) {
            updateTabLabel(editor);
        }
    }
}

QString EditorManager::pathForWidget(QWidget *widget) const
{
    if (!widget) {
        return {};
    }
    if (auto *editor = qobject_cast<CodeEditor *>(widget)) {
        if (auto *doc = EditorDocument::fromEditor(editor)) {
            return doc->filePath();
        }
    }
    return widget->property("filePath").toString();
}

QList<CodeEditor *> EditorManager::modifiedEditors() const
{
    QList<CodeEditor *> result;
    QList<EditorDocument *> seen;
    for (CodeEditor *editor : editors()) {
        auto *doc = EditorDocument::fromEditor(editor);
        if (!doc || !doc->isModified() || seen.contains(doc)) {
            continue;
        }
        seen.append(doc);
        result.append(editor);
    }
    return result;
}

void EditorManager::applySettings()
{
    for (CodeEditor *editor : editors()) {
        EditorStyle::apply(editor);
    }
}

void EditorManager::setLineNumbersVisible(bool visible)
{
    for (CodeEditor *editor : editors()) {
        editor->setLineNumbersVisible(visible);
    }
}

void EditorManager::adjustFontSize(int delta)
{
    if (delta == 0) {
        return;
    }
    AppSettings settings;
    settings.setEditorFontSize(settings.editorFontSize() + delta);
    applySettings();
}

void EditorManager::resetFontSize()
{
    AppSettings().setEditorFontSize(13);
    applySettings();
}

bool EditorManager::goToLine(int lineNumber, int column)
{
    CodeEditor *editor = currentEditor();
    if (!editor || lineNumber < 1) {
        return false;
    }

    QTextBlock block = editor->document()->findBlockByNumber(lineNumber - 1);
    if (!block.isValid()) {
        block = editor->document()->lastBlock();
    }
    if (!block.isValid()) {
        return false;
    }

    editor->unfoldLine(block.blockNumber());
    block = editor->document()->findBlockByNumber(block.blockNumber());

    const int lineLen = qMax(0, block.length() - 1);
    const int offset = qBound(0, column, lineLen);
    QTextCursor cursor = editor->textCursor();
    cursor.setPosition(block.position() + offset);
    editor->setTextCursor(cursor);
    editor->centerCursor();
    return true;
}

bool EditorManager::revealLocation(const QString &filePath, int line, int character)
{
    if (filePath.isEmpty()) {
        return false;
    }
    if (!activateExisting(filePath) && !openTextFile(filePath)) {
        return false;
    }
    return goToLine(line + 1, character);
}

QList<CodeEditor *> EditorManager::editors() const
{
    QList<CodeEditor *> result;
    if (!m_area) {
        return result;
    }
    for (EditorGroup *group : m_area->groups()) {
        for (int i = 0; i < group->tabWidget()->count(); ++i) {
            if (auto *editor = qobject_cast<CodeEditor *>(group->tabWidget()->widget(i))) {
                result.append(editor);
            }
        }
    }
    return result;
}

void EditorManager::saveDirtyFilesQuietly()
{
    for (CodeEditor *editor : modifiedEditors()) {
        auto *doc = EditorDocument::fromEditor(editor);
        if (!doc || doc->isUntitled() || doc->isReadOnly()) {
            continue;
        }
        save(editor);
    }
}

QList<BackupBuffer> EditorManager::dirtyBuffers() const
{
    QList<BackupBuffer> result;
    for (CodeEditor *editor : modifiedEditors()) {
        auto *doc = EditorDocument::fromEditor(editor);
        if (!doc || doc->isReadOnly()) {
            continue;
        }

        BackupBuffer buffer;
        buffer.id = doc->ensureBackupId();
        buffer.originalPath = doc->filePath();
        buffer.displayName = doc->displayName();
        buffer.lfText = editor->toPlainText();
        buffer.format = doc->format();
        const EditorDocument::CaretState caret = doc->caretState();
        buffer.caretPosition = caret.position;
        buffer.caretAnchor = caret.anchor;
        result.append(buffer);
    }
    return result;
}

bool EditorManager::restoreBuffer(const BackupBuffer &buffer)
{
    CodeEditor *editor = nullptr;
    if (!buffer.originalPath.isEmpty()) {
        editor = editorForPath(buffer.originalPath);
        if (!editor && QFileInfo::exists(buffer.originalPath)) {
            if (!openTextFile(buffer.originalPath)) {
                return false;
            }
            editor = editorForPath(buffer.originalPath);
        }
    }

    if (!editor) {
        editor = openUntitled();
    }
    if (!editor) {
        return false;
    }

    auto *doc = EditorDocument::fromEditor(editor);
    if (doc) {
        if (!buffer.id.isEmpty()) {
            doc->setBackupId(buffer.id);
        }
        if (!buffer.originalPath.isEmpty()
            && doc->filePath() != EditorDocument::normalizePath(buffer.originalPath)) {
            doc->setFilePath(buffer.originalPath);
        }
        doc->setFormat(buffer.format);
    }

    if (editor->toPlainText() != buffer.lfText) {
        editor->setPlainText(buffer.lfText);
        editor->document()->setModified(true);
    }

    if (doc) {
        EditorDocument::CaretState caret;
        caret.position = buffer.caretPosition;
        caret.anchor = buffer.caretAnchor;
        doc->restoreCaretState(caret);
    }
    updateTabLabel(editor);
    return true;
}

CodeEditor *EditorManager::openUntitled()
{
    auto *editor = createEditor();
    const int idx = tabWidget()->addTab(editor, tr("untitled"));
    tabWidget()->setCurrentIndex(idx);
    updateTabLabel(editor);
    qCInfo(lcEditor) << "Opened untitled document";
    emit documentOpened(editor);
    return editor;
}

bool EditorManager::activateExisting(const QString &filePath)
{
    if (filePath.isEmpty() || !m_area) {
        return false;
    }
    const QString normalized = EditorDocument::normalizePath(filePath);
    for (EditorGroup *group : m_area->groups()) {
        for (int i = 0; i < group->tabWidget()->count(); ++i) {
            if (pathForWidget(group->tabWidget()->widget(i)) == normalized) {
                setActiveGroup(group);
                group->tabWidget()->setCurrentIndex(i);
                if (auto *editor = currentEditor()) {
                    editor->setFocus();
                }
                return true;
            }
        }
    }
    return false;
}

bool EditorManager::activateWidget(QWidget *widget)
{
    EditorGroup *group = groupForWidget(widget);
    if (!group || !widget) {
        return false;
    }
    setActiveGroup(group);
    group->tabWidget()->setCurrentWidget(widget);
    widget->setFocus();
    return true;
}

QList<QWidget *> EditorManager::tabWidgets() const
{
    QList<QWidget *> widgets;
    if (!m_area) {
        return widgets;
    }
    for (EditorGroup *group : m_area->groups()) {
        QTabWidget *tabs = group->tabWidget();
        for (int i = 0; i < tabs->count(); ++i) {
            widgets.append(tabs->widget(i));
        }
    }
    return widgets;
}

bool EditorManager::openTextFile(const QString &filePath, bool preview)
{
    if (filePath.isEmpty()) {
        return false;
    }

    if (activateExisting(filePath)) {
        if (!preview) {
            promoteCurrentTab();
        }
        if (auto *editor = currentEditor()) {
            editor->setFocus();
        }
        return true;
    }

    const QFileInfo info(filePath);
    if (!info.exists() || !info.isFile()) {
        QMessageBox::warning(m_dialogParent, tr("Error"),
                             tr("Could not open file:\n%1").arg(filePath));
        return false;
    }

    const qint64 size = info.size();
    const AppSettings settings;
    if (size > settings.largeFileWarnBytes()) {
        const auto result = QMessageBox::question(
            m_dialogParent,
            tr("Large file"),
            tr("The file \"%1\" is %2 MB. Opening it may be slow. Continue?")
                .arg(info.fileName())
                .arg(static_cast<double>(size) / (1024.0 * 1024.0), 0, 'f', 1),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::Yes);
        if (result != QMessageBox::Yes) {
            return false;
        }
    }

    const TextLoadResult loaded = readTextFile(filePath);
    if (!loaded.ok) {
        QMessageBox::warning(m_dialogParent, tr("Error"),
                             tr("Could not open file:\n%1").arg(filePath));
        return false;
    }

    auto *editor = createEditor();
    auto *doc = EditorDocument::fromEditor(editor);
    editor->setPlainText(loaded.text);
    if (doc) {
        doc->setFormat(loaded.meta);
        doc->setFilePath(filePath);
        const bool writable = info.isWritable();
        doc->setReadOnly(!writable);
        editor->setReadOnly(!writable);
        doc->resetVersion();
    }
    editor->document()->setModified(false);
    const QString name = doc ? doc->displayName() : tr("untitled");
    editor->setProperty(kTabBaseLabel, name);

    replacePreviewIfNeeded(preview);
    const int idx = tabWidget()->addTab(editor, name);
    tabWidget()->setCurrentWidget(editor);
    applyTabMode(idx, preview);
    editor->setFocus();

    qCInfo(lcEditor) << "Opened" << doc->filePath();
    emit documentOpened(editor);
    return true;
}

void EditorManager::addViewerTab(QWidget *widget, const QString &filePath, bool preview)
{
    if (!widget || !tabWidget()) {
        return;
    }
    const QString normalized = EditorDocument::normalizePath(filePath);
    widget->setProperty("filePath", normalized);
    QString label = widget->windowTitle();
    if (label.isEmpty()) {
        label = QFileInfo(filePath).fileName();
    }
    widget->setProperty(kTabBaseLabel, label);
    replacePreviewIfNeeded(preview);
    const int idx = tabWidget()->addTab(widget, label);
    tabWidget()->setCurrentIndex(idx);
    applyTabMode(idx, preview);
}

void EditorManager::promoteCurrentTab()
{
    if (!m_active || !tabWidget()) {
        return;
    }
    const int index = tabWidget()->currentIndex();
    if (index < 0) {
        return;
    }
    m_active->promote(index);
    refreshGroupTabs(m_active);
}

void EditorManager::togglePinCurrentTab()
{
    if (!m_active || !tabWidget()) {
        return;
    }
    const int index = tabWidget()->currentIndex();
    if (index < 0) {
        return;
    }
    m_active->setPinned(index, !m_active->isPinned(index));
    refreshGroupTabs(m_active);
}

void EditorManager::copyCurrentPath()
{
    const QString path = currentFilePath();
    if (path.isEmpty() || !qApp) {
        return;
    }
    qApp->clipboard()->setText(QDir::toNativeSeparators(path));
}

void EditorManager::revealCurrentInOs()
{
    const QString path = currentFilePath();
    if (path.isEmpty()) {
        return;
    }
    const QFileInfo info(path);
    const QString target = info.isDir() ? info.absoluteFilePath() : info.absolutePath();
    QDesktopServices::openUrl(QUrl::fromLocalFile(target));
}

void EditorManager::replacePreviewIfNeeded(bool preview)
{
    if (!preview || !m_active) {
        return;
    }
    const int existing = m_active->previewIndex();
    if (existing < 0) {
        return;
    }
    QWidget *widget = m_active->tabWidget()->widget(existing);
    if (auto *editor = qobject_cast<CodeEditor *>(widget)) {
        if (auto *doc = EditorDocument::fromEditor(editor); doc && doc->isModified()) {
            m_active->promote(existing);
            refreshGroupTabs(m_active);
            return;
        }
    }
    closeInGroup(m_active, existing);
}

void EditorManager::applyTabMode(int index, bool preview)
{
    if (!m_active) {
        return;
    }
    m_active->setPreview(index, preview);
    refreshGroupTabs(m_active);
}

QStringList EditorManager::openFilePaths() const
{
    QStringList paths;
    if (!m_area) {
        return paths;
    }
    for (EditorGroup *group : m_area->groups()) {
        for (int i = 0; i < group->tabWidget()->count(); ++i) {
            const QString path = pathForWidget(group->tabWidget()->widget(i));
            if (!path.isEmpty() && !paths.contains(path)) {
                paths.append(path);
            }
        }
    }
    return paths;
}

CodeEditor *EditorManager::editorForPath(const QString &filePath) const
{
    const QList<CodeEditor *> matches = editorsForPath(filePath);
    return matches.isEmpty() ? nullptr : matches.front();
}

QList<CodeEditor *> EditorManager::editorsForPath(const QString &filePath) const
{
    QList<CodeEditor *> result;
    if (filePath.isEmpty()) {
        return result;
    }
    const QString normalized = EditorDocument::normalizePath(filePath);
    for (CodeEditor *editor : editors()) {
        if (pathForWidget(editor) == normalized) {
            result.append(editor);
        }
    }
    return result;
}

bool EditorManager::reloadFromDisk(const QString &filePath)
{
    CodeEditor *editor = editorForPath(filePath);
    if (!editor) {
        return false;
    }

    const TextLoadResult loaded = readTextFile(filePath);
    if (!loaded.ok) {
        return false;
    }

    editor->setPlainText(loaded.text);
    auto *doc = EditorDocument::fromEditor(editor);
    if (doc) {
        doc->setFormat(loaded.meta);
        const bool writable = QFileInfo(filePath).isWritable();
        doc->setReadOnly(!writable);
        for (CodeEditor *view : editorsForPath(filePath)) {
            view->setReadOnly(!writable);
        }
        doc->resetVersion();
        editor->document()->setModified(false);
        updateTabLabelsFor(doc);
    } else {
        editor->document()->setModified(false);
        updateTabLabel(editor);
    }
    qCInfo(lcEditor) << "Reloaded" << filePath;
    return true;
}

void EditorManager::closeUntitledIfPristine()
{
    QList<QWidget *> toClose;
    for (CodeEditor *editor : editors()) {
        auto *doc = EditorDocument::fromEditor(editor);
        if (doc && doc->isUntitled() && !doc->isModified()) {
            toClose.append(editor);
        }
    }
    for (QWidget *widget : toClose) {
        closeWidget(widget);
    }
}

bool EditorManager::writeToDisk(CodeEditor *editor, const QString &path)
{
    emit aboutToSave(path);

    auto *doc = EditorDocument::fromEditor(editor);
    const TextFileMeta meta = doc ? doc->format() : defaultTextFileMeta();
    const TextSaveResult saved = writeTextFile(path, editor->toPlainText(), meta);
    if (!saved.ok) {
        QMessageBox::warning(m_dialogParent, tr("Error"),
                             tr("Could not save file: %1").arg(saved.error));
        return false;
    }

    if (doc) {
        doc->setFormat(saved.meta);
    }
    editor->document()->setModified(false);
    updateTabLabelsFor(doc);
    emit fileSaved(path);
    qCInfo(lcEditor) << "Saved" << path;
    return true;
}

bool EditorManager::saveAs(CodeEditor *editor)
{
    if (!editor) {
        return false;
    }

    QString suggestedDir = m_workingDirectory;
    if (auto *doc = EditorDocument::fromEditor(editor); doc && !doc->isUntitled()) {
        suggestedDir = QFileInfo(doc->filePath()).absolutePath();
    }

    const QString fileName = QFileDialog::getSaveFileName(
        m_dialogParent,
        tr("Save File"),
        suggestedDir,
        tr("All Files (*.*)"));
    if (fileName.isEmpty()) {
        return false;
    }

    if (!writeToDisk(editor, fileName)) {
        return false;
    }

    if (auto *doc = EditorDocument::fromEditor(editor)) {
        doc->setFilePath(fileName);
        emit documentOpened(editor);
    }
    return true;
}

bool EditorManager::save(CodeEditor *editor)
{
    if (!editor) {
        return false;
    }

    auto *doc = EditorDocument::fromEditor(editor);
    if (!doc || doc->isUntitled()) {
        return saveAs(editor);
    }
    return writeToDisk(editor, doc->filePath());
}

bool EditorManager::saveCurrent()
{
    return save(currentEditor());
}

bool EditorManager::saveCurrentAs()
{
    return saveAs(currentEditor());
}

bool EditorManager::saveAll()
{
    for (CodeEditor *editor : modifiedEditors()) {
        if (!save(editor)) {
            return false;
        }
    }
    return true;
}

bool EditorManager::confirmClose(CodeEditor *editor)
{
    if (!editor || !editor->document()->isModified()) {
        return true;
    }

    QMessageBox msg(m_dialogParent);
    msg.setWindowTitle(tr("Save Changes"));
    msg.setText(tr("The document has been modified. Do you want to save your changes?"));
    msg.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    msg.setDefaultButton(QMessageBox::Save);
    const int res = msg.exec();

    if (res == QMessageBox::Save) {
        return save(editor);
    }
    if (res == QMessageBox::Cancel) {
        return false;
    }
    return true;
}

int EditorManager::viewCount(EditorDocument *doc) const
{
    if (!doc) {
        return 0;
    }
    int count = 0;
    for (CodeEditor *editor : editors()) {
        if (EditorDocument::fromEditor(editor) == doc) {
            ++count;
        }
    }
    return count;
}

EditorGroup *EditorManager::groupForWidget(QWidget *widget) const
{
    for (QWidget *current = widget; current; current = current->parentWidget()) {
        if (auto *group = qobject_cast<EditorGroup *>(current)) {
            return group;
        }
    }
    return nullptr;
}

EditorGroup *EditorManager::nextGroup(EditorGroup *group) const
{
    if (!m_area) {
        return nullptr;
    }
    const QList<EditorGroup *> groups = m_area->groups();
    if (groups.isEmpty()) {
        return nullptr;
    }
    const int index = groups.indexOf(group);
    if (index < 0) {
        return groups.front();
    }
    return groups.at((index + 1) % groups.size());
}

void EditorManager::pruneEmptyGroup(EditorGroup *group)
{
    if (!group || !m_area || group->tabWidget()->count() > 0) {
        return;
    }
    if (m_area->groupCount() <= 1) {
        return;
    }
    EditorGroup *fallback = nextGroup(group);
    if (fallback == group) {
        fallback = nullptr;
        for (EditorGroup *candidate : m_area->groups()) {
            if (candidate != group) {
                fallback = candidate;
                break;
            }
        }
    }
    const bool wasActive = (m_active == group);
    m_area->removeGroup(group);
    if (wasActive) {
        m_active = nullptr;
        setActiveGroup(fallback);
    }
}

EditorManager::CloseResult EditorManager::closeTab(int index)
{
    return closeInGroup(m_active, index);
}

EditorManager::CloseResult EditorManager::closeWidget(QWidget *widget)
{
    EditorGroup *group = groupForWidget(widget);
    if (!group) {
        return CloseResult::Closed;
    }
    return closeInGroup(group, group->tabWidget()->indexOf(widget));
}

EditorManager::CloseResult EditorManager::closeInGroup(EditorGroup *group, int index)
{
    if (!group || index < 0 || index >= group->tabWidget()->count()) {
        return CloseResult::Closed;
    }

    QWidget *widget = group->tabWidget()->widget(index);
    auto *editor = qobject_cast<CodeEditor *>(widget);
    auto *doc = EditorDocument::fromEditor(editor);
    const bool lastView = editor && viewCount(doc) <= 1;

    if (editor && lastView && !confirmClose(editor)) {
        return CloseResult::Cancelled;
    }
    if (editor && lastView) {
        emit documentAboutToClose(editor);
    }
    if (editor && !lastView && doc) {
        for (CodeEditor *view : editors()) {
            if (view != editor && EditorDocument::fromEditor(view) == doc) {
                doc->setEditor(view);
                break;
            }
        }
    }

    group->tabWidget()->removeTab(index);
    if (editor && lastView && doc && doc->parent() == this) {
        editor->setDocument(new QTextDocument(editor));
        doc->deleteLater();
    }
    if (widget) {
        widget->deleteLater();
    }
    pruneEmptyGroup(group);
    emit currentChanged();
    return CloseResult::Closed;
}

EditorManager::CloseResult EditorManager::closeCurrent()
{
    return tabWidget() ? closeTab(tabWidget()->currentIndex()) : CloseResult::Closed;
}

EditorManager::CloseResult EditorManager::closeWidgets(const QList<QWidget *> &widgets)
{
    for (QWidget *widget : widgets) {
        if (closeWidget(widget) == CloseResult::Cancelled) {
            return CloseResult::Cancelled;
        }
    }
    return CloseResult::Closed;
}

EditorManager::CloseResult EditorManager::closeOthers()
{
    if (!m_active || !tabWidget()) {
        return CloseResult::Closed;
    }
    const QList<int> indices =
        tabIndicesCloseOthers(tabWidget()->currentIndex(), flagsForGroup(m_active));
    QList<QWidget *> toClose;
    for (int index : indices) {
        toClose.append(m_active->tabWidget()->widget(index));
    }
    return closeWidgets(toClose);
}

EditorManager::CloseResult EditorManager::closeToRight()
{
    if (!m_active || !tabWidget()) {
        return CloseResult::Closed;
    }
    const QList<int> indices =
        tabIndicesCloseToRight(tabWidget()->currentIndex(), flagsForGroup(m_active));
    QList<QWidget *> toClose;
    for (int index : indices) {
        toClose.append(m_active->tabWidget()->widget(index));
    }
    return closeWidgets(toClose);
}

EditorManager::CloseResult EditorManager::closeSaved()
{
    if (!m_active) {
        return CloseResult::Closed;
    }
    const QList<int> indices = tabIndicesCloseSaved(flagsForGroup(m_active));
    QList<QWidget *> toClose;
    for (int index : indices) {
        toClose.append(m_active->tabWidget()->widget(index));
    }
    return closeWidgets(toClose);
}

EditorManager::CloseResult EditorManager::closeAll()
{
    QList<QWidget *> toClose;
    for (CodeEditor *editor : editors()) {
        toClose.append(editor);
    }
    if (m_area) {
        for (EditorGroup *group : m_area->groups()) {
            for (int i = 0; i < group->tabWidget()->count(); ++i) {
                QWidget *widget = group->tabWidget()->widget(i);
                if (widget && !toClose.contains(widget)) {
                    toClose.append(widget);
                }
            }
        }
    }
    for (QWidget *widget : toClose) {
        if (closeWidget(widget) == CloseResult::Cancelled) {
            return CloseResult::Cancelled;
        }
    }
    return CloseResult::Closed;
}

void EditorManager::splitRight()
{
    split(Qt::Horizontal);
}

void EditorManager::splitDown()
{
    split(Qt::Vertical);
}

void EditorManager::split(Qt::Orientation orientation)
{
    QWidget *current = currentWidget();
    if (!current || !m_active) {
        return;
    }

    auto *newGroup = createGroup();
    m_area->split(m_active, newGroup, orientation);
    setActiveGroup(newGroup);

    if (auto *editor = qobject_cast<CodeEditor *>(current)) {
        auto *clone = cloneEditorView(editor);
        auto *doc = EditorDocument::fromEditor(editor);
        QString label = doc ? doc->displayName() : tr("untitled");
        if (doc && doc->isModified()) {
            label += QLatin1Char('*');
        }
        const int idx = newGroup->tabWidget()->addTab(clone, label);
        newGroup->tabWidget()->setTabToolTip(idx, doc ? doc->filePath() : QString());
        newGroup->tabWidget()->setCurrentIndex(idx);
        updateTabLabel(clone);
        clone->setFocus();
        emit documentOpened(clone);
        return;
    }

    const QString path = pathForWidget(current);
    if (!path.isEmpty()) {
        emit viewerDuplicateRequested(path);
    }
    if (newGroup->tabWidget()->count() == 0) {
        openUntitled();
    }
}

void EditorManager::moveCurrentToGroup(EditorGroup *target)
{
    QWidget *widget = currentWidget();
    if (!widget || !m_active || !target || target == m_active) {
        return;
    }

    QTabWidget *sourceTabs = m_active->tabWidget();
    const int index = sourceTabs->indexOf(widget);
    if (index < 0) {
        return;
    }
    const QString text = sourceTabs->tabText(index);
    const QString tip = sourceTabs->tabToolTip(index);
    sourceTabs->removeTab(index);
    const int dest = target->tabWidget()->addTab(widget, text);
    target->tabWidget()->setTabToolTip(dest, tip);
    target->tabWidget()->setCurrentIndex(dest);
    if (target->isPreview(dest)) {
        target->setPreview(dest, true);
    }
    target->enforcePinOrder();
    refreshGroupTabs(target);
    pruneEmptyGroup(m_active);
    setActiveGroup(target);
    widget->setFocus();
}

void EditorManager::moveEditor()
{
    if (!currentWidget() || !m_active) {
        return;
    }
    if (groupCount() < 2) {
        auto *source = m_active;
        QWidget *widget = currentWidget();
        auto *newGroup = createGroup();
        QTabWidget *sourceTabs = source->tabWidget();
        const int index = sourceTabs->indexOf(widget);
        const QString text = sourceTabs->tabText(index);
        const QString tip = sourceTabs->tabToolTip(index);
        sourceTabs->removeTab(index);
        m_area->split(source, newGroup, Qt::Horizontal);
        const int dest = newGroup->tabWidget()->addTab(widget, text);
        newGroup->tabWidget()->setTabToolTip(dest, tip);
        newGroup->tabWidget()->setCurrentIndex(dest);
        if (newGroup->isPreview(dest)) {
            newGroup->setPreview(dest, true);
        }
        newGroup->enforcePinOrder();
        refreshGroupTabs(newGroup);
        pruneEmptyGroup(source);
        setActiveGroup(newGroup);
        widget->setFocus();
        return;
    }
    moveCurrentToGroup(nextGroup(m_active));
}

void EditorManager::closeActiveGroup()
{
    if (!m_active) {
        return;
    }
    if (groupCount() <= 1) {
        closeAll();
        return;
    }

    EditorGroup *group = m_active;
    QList<QWidget *> widgets;
    for (int i = 0; i < group->tabWidget()->count(); ++i) {
        widgets.append(group->tabWidget()->widget(i));
    }
    for (QWidget *widget : widgets) {
        if (closeWidget(widget) == CloseResult::Cancelled) {
            return;
        }
    }
}

void EditorManager::showTabContextMenu(EditorGroup *group, int index, const QPoint &globalPos)
{
    if (!group) {
        return;
    }
    if (index >= 0) {
        group->tabWidget()->setCurrentIndex(index);
    }

    QMenu menu(m_dialogParent);
    const bool pinned = index >= 0 && group->isPinned(index);
    QAction *pinAct = menu.addAction(pinned ? tr("Unpin") : tr("Pin"));
    pinAct->setEnabled(index >= 0);
    menu.addSeparator();
    QAction *closeAct = menu.addAction(tr("Close"));
    QAction *closeOthersAct = menu.addAction(tr("Close Others"));
    QAction *closeToRightAct = menu.addAction(tr("Close to the Right"));
    QAction *closeSavedAct = menu.addAction(tr("Close Saved"));
    QAction *closeAllAct = menu.addAction(tr("Close All"));
    menu.addSeparator();
    QAction *copyPathAct = menu.addAction(tr("Copy Path"));
    QAction *revealAct = menu.addAction(tr("Reveal"));
    const bool hasPath = !currentFilePath().isEmpty();
    copyPathAct->setEnabled(hasPath);
    revealAct->setEnabled(hasPath);
    menu.addSeparator();
    QAction *splitRightAct = menu.addAction(tr("Split Right"));
    QAction *splitDownAct = menu.addAction(tr("Split Down"));
    QAction *moveAct = menu.addAction(tr("Move Editor"));
    QAction *closeGroupAct = menu.addAction(tr("Close Group"));
    closeGroupAct->setEnabled(groupCount() > 1);

    QAction *chosen = menu.exec(globalPos);
    if (chosen == pinAct) {
        togglePinCurrentTab();
    } else if (chosen == closeAct) {
        closeCurrent();
    } else if (chosen == closeOthersAct) {
        closeOthers();
    } else if (chosen == closeToRightAct) {
        closeToRight();
    } else if (chosen == closeSavedAct) {
        closeSaved();
    } else if (chosen == closeAllAct) {
        closeAll();
    } else if (chosen == copyPathAct) {
        copyCurrentPath();
    } else if (chosen == revealAct) {
        revealCurrentInOs();
    } else if (chosen == splitRightAct) {
        splitRight();
    } else if (chosen == splitDownAct) {
        splitDown();
    } else if (chosen == moveAct) {
        moveEditor();
    } else if (chosen == closeGroupAct) {
        closeActiveGroup();
    }
}

bool EditorManager::promptSaveAllOnQuit()
{
    return promptSaveEditors(modifiedEditors());
}

bool EditorManager::promptSaveEditors(const QList<CodeEditor *> &editors, bool secretFiles)
{
    if (editors.isEmpty()) {
        return true;
    }

    QMessageBox msg(m_dialogParent);
    msg.setWindowTitle(tr("Unsaved Changes"));
    if (secretFiles) {
        msg.setText(tr("You have %1 unsaved secret file(s) that cannot be backed up.\n"
                       "Do you want to save them before closing?")
                        .arg(editors.size()));
    } else {
        msg.setText(tr("You have %1 file(s) with unsaved changes.\n"
                       "Do you want to save all changes before closing?")
                        .arg(editors.size()));
    }
    msg.setStandardButtons(QMessageBox::SaveAll | QMessageBox::Discard | QMessageBox::Cancel);
    msg.setDefaultButton(QMessageBox::SaveAll);
    msg.setIcon(QMessageBox::Warning);

    const int res = msg.exec();
    if (res == QMessageBox::SaveAll) {
        for (CodeEditor *editor : editors) {
            if (!save(editor)) {
                return false;
            }
        }
        return true;
    }
    if (res == QMessageBox::Discard) {
        return true;
    }
    return false;
}
