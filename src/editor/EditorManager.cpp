#include "editor/EditorManager.h"

#include "editor/CodeEditor.h"
#include "core/AppSettings.h"
#include "core/AtomicFile.h"
#include "core/Logging.h"
#include "editor/EditorDocument.h"
#include "syntax/LanguageRegistry.h"
#include "syntax/SyntaxHighlighter.h"

#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QFontMetrics>
#include <QtGlobal>
#include <QMessageBox>
#include <QTabWidget>
#include <QTextStream>

EditorManager::EditorManager(QWidget *dialogParent)
    : QObject(dialogParent)
    , m_dialogParent(dialogParent)
    , m_tabs(new QTabWidget(dialogParent))
{
    m_tabs->setTabsClosable(true);
    m_tabs->setMovable(true);

    connect(m_tabs, &QTabWidget::currentChanged, this, [this](int) {
        emit currentChanged();
    });
    connect(m_tabs, &QTabWidget::tabCloseRequested, this, [this](int index) {
        closeTab(index);
    });

    openUntitled();
}

CodeEditor *EditorManager::currentEditor() const
{
    return qobject_cast<CodeEditor *>(m_tabs->currentWidget());
}

EditorDocument *EditorManager::currentDocument() const
{
    return EditorDocument::fromEditor(currentEditor());
}

QWidget *EditorManager::currentWidget() const
{
    return m_tabs->currentWidget();
}

QString EditorManager::currentFilePath() const
{
    return pathForWidget(m_tabs->currentWidget());
}

void EditorManager::setWorkingDirectory(const QString &dir)
{
    m_workingDirectory = dir;
}

CodeEditor *EditorManager::createEditor()
{
    auto *editor = new CodeEditor(m_tabs);
    applyEditorStyle(editor);

    auto *doc = new EditorDocument(editor);
    connect(doc, &EditorDocument::modificationChanged, this, [this, editor](bool) {
        updateTabLabel(editor);
        emit modificationChanged();
    });
    connect(doc, &EditorDocument::filePathChanged, this, [this, editor](const QString &) {
        updateTabLabel(editor);
        syncHighlighter(editor);
    });

    return editor;
}

void EditorManager::applyEditorStyle(CodeEditor *editor) const
{
    const AppSettings settings;
    QFont font(settings.editorFontFamily());
    font.setPointSize(qMax(8, settings.editorFontSize()));
    editor->setFont(font);

    const QFontMetrics metrics(font);
    editor->setTabStopDistance(
        settings.editorTabSize() * metrics.horizontalAdvance(QLatin1Char(' ')));
    editor->setLineWrapMode(settings.editorWordWrap()
                                ? QPlainTextEdit::WidgetWidth
                                : QPlainTextEdit::NoWrap);
    editor->setLineNumbersVisible(settings.editorLineNumbers());
}

void EditorManager::syncHighlighter(CodeEditor *editor)
{
    if (!editor) {
        return;
    }

    auto *edDoc = EditorDocument::fromEditor(editor);
    const QString path = edDoc ? edDoc->filePath() : QString();
    const LanguageId lang = LanguageRegistry::idForPath(path);

    qint64 size = static_cast<qint64>(editor->document()->characterCount());
    if (!path.isEmpty()) {
        const QFileInfo info(path);
        if (info.exists()) {
            size = info.size();
        }
    }

    const auto existing = editor->document()->findChildren<SyntaxHighlighter *>(
        QString(), Qt::FindDirectChildrenOnly);

    const bool tooLarge = size > AppSettings().largeFileDisableSyntaxBytes();
    const bool canHighlight = LanguageRegistry::tsLanguage(lang) != nullptr;
    if (tooLarge || !canHighlight) {
        for (SyntaxHighlighter *highlighter : existing) {
            delete highlighter;
        }
        if (tooLarge) {
            qCInfo(lcEditor) << "Skipping syntax highlighter for large file" << path << size;
        }
        return;
    }

    if (existing.size() == 1 && existing.front()->language() == lang) {
        return;
    }

    for (SyntaxHighlighter *highlighter : existing) {
        delete highlighter;
    }

    qCInfo(lcEditor) << "Highlighter" << LanguageRegistry::displayName(lang) << "for" << path;
    new SyntaxHighlighter(editor->document(), lang);
}

void EditorManager::updateTabLabel(CodeEditor *editor)
{
    if (!editor) {
        return;
    }
    const int idx = m_tabs->indexOf(editor);
    if (idx < 0) {
        return;
    }

    auto *doc = EditorDocument::fromEditor(editor);
    QString label = doc ? doc->displayName() : tr("untitled");
    if (doc && doc->isModified()) {
        label += QLatin1Char('*');
    }
    m_tabs->setTabText(idx, label);
    m_tabs->setTabToolTip(idx, doc ? doc->filePath() : QString());
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
    for (int i = 0; i < m_tabs->count(); ++i) {
        auto *editor = qobject_cast<CodeEditor *>(m_tabs->widget(i));
        if (editor && editor->document()->isModified()) {
            result.append(editor);
        }
    }
    return result;
}

CodeEditor *EditorManager::openUntitled()
{
    auto *editor = createEditor();
    const int idx = m_tabs->addTab(editor, tr("untitled"));
    m_tabs->setCurrentIndex(idx);
    updateTabLabel(editor);
    qCInfo(lcEditor) << "Opened untitled document";
    return editor;
}

bool EditorManager::activateExisting(const QString &filePath)
{
    if (filePath.isEmpty()) {
        return false;
    }
    const QString normalized = EditorDocument::normalizePath(filePath);
    for (int i = 0; i < m_tabs->count(); ++i) {
        if (pathForWidget(m_tabs->widget(i)) == normalized) {
            m_tabs->setCurrentIndex(i);
            return true;
        }
    }
    return false;
}

bool EditorManager::openTextFile(const QString &filePath)
{
    if (filePath.isEmpty()) {
        return false;
    }

    if (activateExisting(filePath)) {
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

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(m_dialogParent, tr("Error"),
                             tr("Could not open file:\n%1").arg(filePath));
        return false;
    }

    QTextStream in(&file);
    const QString content = in.readAll();
    file.close();

    auto *editor = createEditor();
    auto *doc = EditorDocument::fromEditor(editor);
    editor->setPlainText(content);
    doc->setFilePath(filePath);
    editor->document()->setModified(false);

    const int idx = m_tabs->addTab(editor, doc->displayName());
    m_tabs->setTabToolTip(idx, doc->filePath());
    m_tabs->setCurrentWidget(editor);
    updateTabLabel(editor);
    editor->setFocus();

    qCInfo(lcEditor) << "Opened" << doc->filePath();
    return true;
}

void EditorManager::addViewerTab(QWidget *widget, const QString &filePath)
{
    if (!widget) {
        return;
    }
    const QString normalized = EditorDocument::normalizePath(filePath);
    widget->setProperty("filePath", normalized);
    const int idx = m_tabs->addTab(widget, QFileInfo(filePath).fileName());
    m_tabs->setTabToolTip(idx, normalized);
    m_tabs->setCurrentIndex(idx);
}

QStringList EditorManager::openFilePaths() const
{
    QStringList paths;
    for (int i = 0; i < m_tabs->count(); ++i) {
        const QString path = pathForWidget(m_tabs->widget(i));
        if (!path.isEmpty()) {
            paths.append(path);
        }
    }
    return paths;
}

CodeEditor *EditorManager::editorForPath(const QString &filePath) const
{
    if (filePath.isEmpty()) {
        return nullptr;
    }
    const QString normalized = EditorDocument::normalizePath(filePath);
    for (int i = 0; i < m_tabs->count(); ++i) {
        auto *editor = qobject_cast<CodeEditor *>(m_tabs->widget(i));
        if (editor && pathForWidget(editor) == normalized) {
            return editor;
        }
    }
    return nullptr;
}

bool EditorManager::reloadFromDisk(const QString &filePath)
{
    CodeEditor *editor = editorForPath(filePath);
    if (!editor) {
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream in(&file);
    const QString content = in.readAll();
    file.close();

    editor->setPlainText(content);
    editor->document()->setModified(false);
    updateTabLabel(editor);
    qCInfo(lcEditor) << "Reloaded" << filePath;
    return true;
}

void EditorManager::closeUntitledIfPristine()
{
    QList<QWidget *> toClose;
    for (int i = 0; i < m_tabs->count(); ++i) {
        auto *editor = qobject_cast<CodeEditor *>(m_tabs->widget(i));
        auto *doc = EditorDocument::fromEditor(editor);
        if (doc && doc->isUntitled() && !doc->isModified()) {
            toClose.append(editor);
        }
    }
    for (QWidget *widget : toClose) {
        const int idx = m_tabs->indexOf(widget);
        if (idx >= 0) {
            closeTab(idx);
        }
    }
}

bool EditorManager::writeToDisk(CodeEditor *editor, const QString &path)
{
    emit aboutToSave(path);

    QString error;
    if (!writeTextAtomically(path, editor->toPlainText(), &error)) {
        QMessageBox::warning(m_dialogParent, tr("Error"),
                             tr("Could not save file: %1").arg(error));
        return false;
    }

    editor->document()->setModified(false);
    updateTabLabel(editor);
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

EditorManager::CloseResult EditorManager::closeTab(int index)
{
    if (index < 0 || index >= m_tabs->count()) {
        return CloseResult::Closed;
    }

    QWidget *widget = m_tabs->widget(index);
    if (auto *editor = qobject_cast<CodeEditor *>(widget)) {
        if (!confirmClose(editor)) {
            return CloseResult::Cancelled;
        }
    }

    m_tabs->removeTab(index);
    if (widget) {
        widget->deleteLater();
    }
    return CloseResult::Closed;
}

EditorManager::CloseResult EditorManager::closeCurrent()
{
    return closeTab(m_tabs->currentIndex());
}

EditorManager::CloseResult EditorManager::closeOthers()
{
    QWidget *keep = m_tabs->currentWidget();
    if (!keep) {
        return CloseResult::Closed;
    }

    QList<QWidget *> toClose;
    for (int i = 0; i < m_tabs->count(); ++i) {
        if (m_tabs->widget(i) != keep) {
            toClose.append(m_tabs->widget(i));
        }
    }

    for (QWidget *widget : toClose) {
        const int idx = m_tabs->indexOf(widget);
        if (idx < 0) {
            continue;
        }
        if (closeTab(idx) == CloseResult::Cancelled) {
            return CloseResult::Cancelled;
        }
    }
    return CloseResult::Closed;
}

EditorManager::CloseResult EditorManager::closeAll()
{
    while (m_tabs->count() > 0) {
        if (closeTab(m_tabs->count() - 1) == CloseResult::Cancelled) {
            return CloseResult::Cancelled;
        }
    }
    return CloseResult::Closed;
}

bool EditorManager::promptSaveAllOnQuit()
{
    const QList<CodeEditor *> modified = modifiedEditors();
    if (modified.isEmpty()) {
        return true;
    }

    QMessageBox msg(m_dialogParent);
    msg.setWindowTitle(tr("Unsaved Changes"));
    msg.setText(tr("You have %1 file(s) with unsaved changes.\n"
                   "Do you want to save all changes before closing?")
                    .arg(modified.size()));
    msg.setStandardButtons(QMessageBox::SaveAll | QMessageBox::Discard | QMessageBox::Cancel);
    msg.setDefaultButton(QMessageBox::SaveAll);
    msg.setIcon(QMessageBox::Warning);

    const int res = msg.exec();
    if (res == QMessageBox::SaveAll) {
        for (CodeEditor *editor : modified) {
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
