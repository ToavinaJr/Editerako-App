#include "app/LspSession.h"

#include "core/Logging.h"
#include "editor/CodeEditor.h"
#include "editor/CompletionModel.h"
#include "editor/CompletionPopup.h"
#include "editor/EditorDiagnostic.h"
#include "editor/EditorDocument.h"
#include "editor/EditorManager.h"
#include "editor/HoverPopup.h"
#include "lsp/LspClient.h"
#include "lsp/LspCompletionProvider.h"
#include "lsp/LspDiagnosticsProvider.h"
#include "lsp/LspDocumentSync.h"
#include "lsp/LspHoverProvider.h"
#include "lsp/LspNavigationProvider.h"
#include "lsp/LspServerManager.h"
#include "lsp/LspSymbolProvider.h"
#include "lsp/LspTypes.h"
#include "syntax/LanguageRegistry.h"
#include "ui/FuzzyPickerDialog.h"

#include <QFileInfo>
#include <QInputDialog>
#include <QLineEdit>
#include <QTextCursor>
#include <QTextDocument>
#include <QTimer>
#include <algorithm>

namespace {

CompletionItem toCompletionItem(const LspCompletionItem &item)
{
    CompletionItem out;
    out.label = item.label;
    out.detail = item.detail;
    out.documentation = item.documentation;
    out.insertText = item.insertText;
    out.sortText = item.sortText;
    out.kind = item.kind;
    out.hasTextEdit = item.hasTextEdit;
    out.startLine = item.textEditRange.start.line;
    out.startCharacter = item.textEditRange.start.character;
    out.endLine = item.textEditRange.end.line;
    out.endCharacter = item.textEditRange.end.character;
    return out;
}

EditorDiagnostic toEditorDiagnostic(const LspDiagnostic &diag)
{
    EditorDiagnostic out;
    out.startLine = diag.range.start.line;
    out.startCharacter = diag.range.start.character;
    out.endLine = diag.range.end.line;
    out.endCharacter = diag.range.end.character;
    switch (diag.severity) {
    case LspSeverity::Warning:
        out.severity = EditorDiagnostic::Severity::Warning;
        break;
    case LspSeverity::Information:
        out.severity = EditorDiagnostic::Severity::Information;
        break;
    case LspSeverity::Hint:
        out.severity = EditorDiagnostic::Severity::Hint;
        break;
    case LspSeverity::Error:
    default:
        out.severity = EditorDiagnostic::Severity::Error;
        break;
    }
    out.message = diag.message;
    if (!diag.source.isEmpty()) {
        out.message = diag.source + QStringLiteral(": ") + diag.message;
    }
    return out;
}

void applyRangeText(QPlainTextEdit *editor, const LspRange &range, const QString &text)
{
    if (!editor) {
        return;
    }
    QTextDocument *doc = editor->document();
    QTextCursor cursor(doc);
    const int start = documentPositionAt(doc, range.start.line, range.start.character);
    const int end = documentPositionAt(doc, range.end.line, range.end.character);
    cursor.setPosition(start);
    cursor.setPosition(qMax(start, end), QTextCursor::KeepAnchor);
    cursor.insertText(text);
}

} // namespace

LspSession::LspSession(LspServerManager *manager, EditorManager *editors, QWidget *dialogParent)
    : QObject(dialogParent)
    , m_manager(manager)
    , m_editors(editors)
    , m_dialogParent(dialogParent)
    , m_completion(new CompletionPopup(dialogParent))
    , m_hover(new HoverPopup(dialogParent))
    , m_changeTimer(new QTimer(this))
{
    m_changeTimer->setSingleShot(true);
    m_changeTimer->setInterval(200);
    connect(m_changeTimer, &QTimer::timeout, this, &LspSession::flushPendingChange);

    if (m_editors) {
        connect(m_editors, &EditorManager::documentOpened, this, &LspSession::onDocumentOpened);
        connect(m_editors, &EditorManager::documentAboutToClose, this,
                &LspSession::onDocumentAboutToClose);
        connect(m_editors, &EditorManager::fileSaved, this, &LspSession::onFileSaved);
        connect(m_editors, &EditorManager::currentChanged, this, [this]() {
            m_completion->hidePopup();
            m_hover->hidePopup();
        });
        for (CodeEditor *editor : m_editors->editors()) {
            attachEditor(editor);
            onDocumentOpened(editor);
        }
    }

    connect(m_completion, &CompletionPopup::itemActivated, this, &LspSession::applyCompletion);
}

void LspSession::setWorkspaceRoot(const QString &root)
{
    m_workspaceRoot = root;
}

void LspSession::attachEditor(CodeEditor *editor)
{
    if (!editor || editor->property("lspAttached").toBool()) {
        return;
    }
    editor->setProperty("lspAttached", true);
    connect(editor, &CodeEditor::completionRequested, this, &LspSession::triggerCompletion);
    connect(editor, &CodeEditor::signatureHelpRequested, this, &LspSession::triggerSignatureHelp);
    connect(editor, &CodeEditor::hoverRequested, this, &LspSession::onHoverRequested);
    if (auto *doc = EditorDocument::fromEditor(editor)) {
        connect(doc, &EditorDocument::versionChanged, this, [this, editor](int) {
            onEditorContentsChanged(editor);
        });
    }
}

void LspSession::onHoverRequested(int line, int character, const QPoint &globalPos)
{
    QString uri;
    int curLine = 0;
    int curChar = 0;
    if (!currentPosition(&uri, &curLine, &curChar)) {
        return;
    }
    if (m_completion->isVisible()) {
        return;
    }
    auto *doc = m_editors ? m_editors->currentDocument() : nullptr;
    if (!doc || !ensureClangd(doc)) {
        return;
    }
    auto *hover = m_manager->hoverForLanguage(languageKey(doc));
    if (!hover) {
        return;
    }
    const int gen = ++m_hoverGeneration;
    hover->hover(uri, line, character, [this, gen, globalPos](const LspHover &result) {
        if (gen != m_hoverGeneration || result.contents.isEmpty()) {
            return;
        }
        m_hover->showMarkdown(result.contents, globalPos);
    });
}

bool LspSession::ensureClangd(EditorDocument *doc)
{
    if (!m_manager || !doc || doc->isUntitled()) {
        return false;
    }
    const LanguageDefinition &def = LanguageRegistry::definition(doc->language());
    if (def.languageServer != QLatin1String("clangd")) {
        return false;
    }

    if (!m_clangdRegistered) {
        LspServerSpec spec;
        spec.id = QStringLiteral("clangd");
        spec.command = QStringLiteral("clangd");
        spec.args = QStringList{QStringLiteral("--offset-encoding=utf-16")};
        spec.languageIds = QStringList{QStringLiteral("c"), QStringLiteral("cpp")};
        m_manager->registerSpec(spec);
        m_clangdRegistered = true;
        connect(m_manager, &LspServerManager::serverFailed, this,
                [this](const QString &specId, const QString &message) {
                    if (specId != QLatin1String("clangd")) {
                        return;
                    }
                    qCWarning(lcLsp) << message;
                    emit lspStatusChanged(tr("clangd error"));
                    if (!m_missingWarned) {
                        m_missingWarned = true;
                        emit statusMessage(
                            tr("clangd is not available. C/C++ language features are disabled."),
                            8000);
                    }
                });
    }

    QString root = m_workspaceRoot;
    if (root.isEmpty()) {
        root = QFileInfo(doc->filePath()).absolutePath();
    }
    if (!m_manager->ensureSpec(QStringLiteral("clangd"), lspFileUri(root), root)) {
        if (!m_missingWarned) {
            m_missingWarned = true;
            emit statusMessage(tr("clangd is not installed. C/C++ language features are disabled."),
                               8000);
            emit lspStatusChanged(tr("clangd missing"));
        }
        return false;
    }

    m_missingWarned = false;
    emit lspStatusChanged(tr("clangd"));

    if (LspClient *client = m_manager->clientForLanguage(QStringLiteral("cpp"))) {
        connect(client, &LspClient::initializedChanged, this, &LspSession::onInitialized,
                Qt::UniqueConnection);
    }
    if (auto *diagnostics = m_manager->diagnosticsForLanguage(QStringLiteral("cpp"))) {
        connect(diagnostics, &LspDiagnosticsProvider::diagnosticsPublished, this,
                &LspSession::onDiagnostics, Qt::UniqueConnection);
    }
    return m_manager->clientForLanguage(QStringLiteral("cpp")) != nullptr
        || m_manager->clientForLanguage(QStringLiteral("c")) != nullptr;
}

QString LspSession::languageKey(EditorDocument *doc) const
{
    if (!doc) {
        return {};
    }
    return LanguageRegistry::languageIdString(doc->language());
}

void LspSession::openOnServer(CodeEditor *editor)
{
    auto *doc = EditorDocument::fromEditor(editor);
    if (!doc || doc->isUntitled() || !ensureClangd(doc)) {
        return;
    }
    const QString lang = languageKey(doc);
    LspDocumentSync *sync = m_manager->documentSyncForLanguage(lang);
    LspClient *client = m_manager->clientForLanguage(lang);
    if (!sync || !client || !client->isInitialized()) {
        return;
    }
    const QString uri = lspFileUri(doc->filePath());
    if (m_openUris.contains(uri)) {
        return;
    }
    sync->didOpen(uri, lang, qMax(1, doc->version()), editor->toPlainText());
    m_openUris.insert(uri);
}

void LspSession::closeOnServer(const QString &path)
{
    if (path.isEmpty() || !m_manager) {
        return;
    }
    const QString uri = lspFileUri(path);
    if (!m_openUris.remove(uri)) {
        return;
    }
    const QString lang = LanguageRegistry::languageIdString(LanguageRegistry::idForPath(path));
    if (auto *sync = m_manager->documentSyncForLanguage(lang)) {
        sync->didClose(uri);
    }
}

void LspSession::onDocumentOpened(CodeEditor *editor)
{
    attachEditor(editor);
    openOnServer(editor);
}

void LspSession::onDocumentAboutToClose(CodeEditor *editor)
{
    m_completion->hidePopup();
    m_hover->hidePopup();
    auto *doc = EditorDocument::fromEditor(editor);
    if (!doc) {
        return;
    }
    const QString path = doc->filePath();
    closeOnServer(path);
    if (editor) {
        editor->setDiagnostics({});
    }
}

void LspSession::onFileSaved(const QString &path)
{
    if (path.isEmpty() || !m_manager) {
        return;
    }
    const QString lang = LanguageRegistry::languageIdString(LanguageRegistry::idForPath(path));
    if (auto *sync = m_manager->documentSyncForLanguage(lang)) {
        if (m_openUris.contains(lspFileUri(path))) {
            sync->didSave(lspFileUri(path));
        }
    }
}

void LspSession::onInitialized(bool initialized)
{
    if (!initialized || !m_editors) {
        return;
    }
    emit lspStatusChanged(tr("clangd"));
    for (CodeEditor *editor : m_editors->editors()) {
        openOnServer(editor);
    }
}

void LspSession::onDiagnostics(const QString &uri, const QVector<LspDiagnostic> &diagnostics)
{
    if (!m_editors) {
        return;
    }
    CodeEditor *editor = m_editors->editorForPath(lspUriToPath(uri));
    if (!editor) {
        return;
    }
    QVector<EditorDiagnostic> mapped;
    mapped.reserve(diagnostics.size());
    for (const LspDiagnostic &diag : diagnostics) {
        mapped.append(toEditorDiagnostic(diag));
    }
    editor->setDiagnostics(mapped);
}

void LspSession::onEditorContentsChanged(CodeEditor *editor)
{
    m_pendingChangeEditor = editor;
    m_changeTimer->start();
    if (m_completion->isVisibleFor(editor)) {
        m_completion->updateFilter(completionPrefixAtCursor(editor->textCursor()));
    }
}

void LspSession::flushPendingChange()
{
    CodeEditor *editor = m_pendingChangeEditor;
    m_pendingChangeEditor = nullptr;
    auto *doc = EditorDocument::fromEditor(editor);
    if (!doc || doc->isUntitled() || !m_manager) {
        return;
    }
    const QString lang = languageKey(doc);
    LspDocumentSync *sync = m_manager->documentSyncForLanguage(lang);
    const QString uri = lspFileUri(doc->filePath());
    if (!sync || !m_openUris.contains(uri)) {
        openOnServer(editor);
        return;
    }
    sync->didChange(uri, qMax(1, doc->version()), editor->toPlainText());
}

bool LspSession::currentPosition(QString *uri, int *line, int *character) const
{
    if (!m_editors || !uri || !line || !character) {
        return false;
    }
    CodeEditor *editor = m_editors->currentEditor();
    auto *doc = EditorDocument::fromEditor(editor);
    if (!doc || doc->isUntitled()) {
        return false;
    }
    *uri = lspFileUri(doc->filePath());
    const QTextCursor cursor = editor->textCursor();
    *line = cursor.blockNumber();
    *character = cursor.positionInBlock();
    return true;
}

void LspSession::triggerCompletion()
{
    m_hover->hidePopup();
    CodeEditor *editor = m_editors ? m_editors->currentEditor() : nullptr;
    auto *doc = EditorDocument::fromEditor(editor);
    if (!doc || !ensureClangd(doc)) {
        return;
    }
    QString uri;
    int line = 0;
    int character = 0;
    if (!currentPosition(&uri, &line, &character)) {
        return;
    }
    const QString lang = languageKey(doc);
    auto *provider = m_manager->completionForLanguage(lang);
    if (!provider) {
        return;
    }
    const int gen = ++m_completionGeneration;
    const QString prefix = completionPrefixAtCursor(editor->textCursor());
    provider->complete(uri, line, character, [this, editor, gen, prefix](const QVector<LspCompletionItem> &items) {
        if (gen != m_completionGeneration || !editor) {
            return;
        }
        QVector<CompletionItem> mapped;
        mapped.reserve(items.size());
        for (const LspCompletionItem &item : items) {
            mapped.append(toCompletionItem(item));
        }
        m_completion->showItems(editor, mapped, prefix);
    });
}

void LspSession::triggerHover()
{
    CodeEditor *editor = m_editors ? m_editors->currentEditor() : nullptr;
    if (!editor) {
        return;
    }
    const QPoint global = editor->mapToGlobal(editor->cursorRect().bottomRight());
    const QTextCursor cursor = editor->textCursor();
    emit editor->hoverRequested(cursor.blockNumber(), cursor.positionInBlock(), global);
}

void LspSession::triggerSignatureHelp()
{
    m_completion->hidePopup();
    auto *doc = m_editors ? m_editors->currentDocument() : nullptr;
    if (!doc || !ensureClangd(doc)) {
        return;
    }
    QString uri;
    int line = 0;
    int character = 0;
    if (!currentPosition(&uri, &line, &character)) {
        return;
    }
    auto *hover = m_manager->hoverForLanguage(languageKey(doc));
    CodeEditor *editor = m_editors->currentEditor();
    if (!hover || !editor) {
        return;
    }
    const QPoint global = editor->mapToGlobal(editor->cursorRect().bottomLeft());
    hover->signatureHelp(uri, line, character, [this, global](const LspSignatureHelp &help) {
        if (!help.valid) {
            return;
        }
        QString text = QStringLiteral("```cpp\n") + help.label + QStringLiteral("\n```");
        if (!help.documentation.isEmpty()) {
            text += QLatin1Char('\n') + help.documentation;
        }
        m_hover->showMarkdown(text, global);
    });
}

void LspSession::applyCompletion(const CompletionItem &item)
{
    if (m_editors) {
        applyCompletionItem(m_editors->currentEditor(), item);
    }
}

void LspSession::goToDefinition()
{
    auto *doc = m_editors ? m_editors->currentDocument() : nullptr;
    if (!doc || !ensureClangd(doc)) {
        return;
    }
    QString uri;
    int line = 0;
    int character = 0;
    if (!currentPosition(&uri, &line, &character)) {
        return;
    }
    auto *nav = m_manager->navigationForLanguage(languageKey(doc));
    if (!nav) {
        return;
    }
    nav->definition(uri, line, character, [this](const QVector<LspLocation> &locations) {
        goToLocations(locations, tr("Go to Definition"));
    });
}

void LspSession::findReferences()
{
    auto *doc = m_editors ? m_editors->currentDocument() : nullptr;
    if (!doc || !ensureClangd(doc)) {
        return;
    }
    QString uri;
    int line = 0;
    int character = 0;
    if (!currentPosition(&uri, &line, &character)) {
        return;
    }
    auto *nav = m_manager->navigationForLanguage(languageKey(doc));
    if (!nav) {
        return;
    }
    nav->references(uri, line, character, [this](const QVector<LspLocation> &locations) {
        goToLocations(locations, tr("Find References"));
    });
}

void LspSession::goToLocations(const QVector<LspLocation> &locations, const QString &title)
{
    if (locations.isEmpty()) {
        emit statusMessage(tr("No results"), 2000);
        return;
    }
    if (locations.size() == 1) {
        const LspLocation &loc = locations.front();
        m_editors->revealLocation(lspUriToPath(loc.uri), loc.range.start.line,
                                  loc.range.start.character);
        return;
    }

    FuzzyPickerDialog dialog(m_dialogParent);
    dialog.setWindowTitle(title);
    dialog.setPlaceholderText(tr("Filter locations..."));
    QList<FuzzyPickerItem> items;
    int index = 0;
    for (const LspLocation &loc : locations) {
        FuzzyPickerItem item;
        item.id = QString::number(index++);
        const QString path = lspUriToPath(loc.uri);
        item.display = QFileInfo(path).fileName();
        item.hint = QStringLiteral("%1:%2").arg(QFileInfo(path).fileName())
                        .arg(loc.range.start.line + 1);
        item.filterText = path;
        items.append(item);
    }
    dialog.setItems(items);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    const int chosen = dialog.selectedId().toInt();
    if (chosen < 0 || chosen >= locations.size()) {
        return;
    }
    const LspLocation &loc = locations.at(chosen);
    m_editors->revealLocation(lspUriToPath(loc.uri), loc.range.start.line, loc.range.start.character);
}

void LspSession::renameSymbol()
{
    auto *doc = m_editors ? m_editors->currentDocument() : nullptr;
    if (!doc || !ensureClangd(doc)) {
        return;
    }
    QString uri;
    int line = 0;
    int character = 0;
    if (!currentPosition(&uri, &line, &character)) {
        return;
    }
    bool ok = false;
    const QString newName = QInputDialog::getText(m_dialogParent, tr("Rename Symbol"),
                                                  tr("New name:"), QLineEdit::Normal, {}, &ok);
    if (!ok || newName.trimmed().isEmpty()) {
        return;
    }
    auto *nav = m_manager->navigationForLanguage(languageKey(doc));
    if (!nav) {
        return;
    }
    nav->rename(uri, line, character, newName.trimmed(), [this](const QJsonObject &edit) {
        applyWorkspaceEdits(edit);
    });
}

void LspSession::applyWorkspaceEdits(const QJsonObject &edit)
{
    QVector<LspTextEdit> edits = lspTextEditsFromWorkspaceEdit(edit);
    if (edits.isEmpty()) {
        emit statusMessage(tr("Nothing to rename"), 2000);
        return;
    }
    std::sort(edits.begin(), edits.end(), [](const LspTextEdit &a, const LspTextEdit &b) {
        if (a.uri != b.uri) {
            return a.uri < b.uri;
        }
        if (a.range.start.line != b.range.start.line) {
            return a.range.start.line > b.range.start.line;
        }
        return a.range.start.character > b.range.start.character;
    });

    QString currentUri;
    CodeEditor *editor = nullptr;
    for (const LspTextEdit &textEdit : edits) {
        if (textEdit.uri != currentUri) {
            currentUri = textEdit.uri;
            const QString path = lspUriToPath(textEdit.uri);
            if (!m_editors->activateExisting(path)) {
                m_editors->openTextFile(path);
            }
            editor = m_editors->editorForPath(path);
        }
        applyRangeText(editor, textEdit.range, textEdit.newText);
    }
}

void LspSession::showDocumentSymbols()
{
    auto *doc = m_editors ? m_editors->currentDocument() : nullptr;
    if (!doc || !ensureClangd(doc)) {
        return;
    }
    auto *symbols = m_manager->symbolsForLanguage(languageKey(doc));
    if (!symbols) {
        return;
    }
    symbols->documentSymbols(lspFileUri(doc->filePath()), [this](const QVector<LspSymbol> &list) {
        showSymbols(list, tr("Document Symbols"));
    });
}

void LspSession::showWorkspaceSymbols()
{
    EditorDocument *doc = m_editors ? m_editors->currentDocument() : nullptr;
    if (doc && !ensureClangd(doc)) {
        return;
    }
    auto *symbols = m_manager ? m_manager->symbolsForLanguage(QStringLiteral("cpp")) : nullptr;
    if (!symbols) {
        symbols = m_manager ? m_manager->symbolsForLanguage(QStringLiteral("c")) : nullptr;
    }
    if (!symbols) {
        emit statusMessage(tr("clangd is not running"), 3000);
        return;
    }
    symbols->workspaceSymbols({}, [this](const QVector<LspSymbol> &list) {
        showSymbols(list, tr("Workspace Symbols"));
    });
}

void LspSession::showSymbols(const QVector<LspSymbol> &symbols, const QString &title)
{
    if (symbols.isEmpty()) {
        emit statusMessage(tr("No symbols"), 2000);
        return;
    }
    FuzzyPickerDialog dialog(m_dialogParent);
    dialog.setWindowTitle(title);
    dialog.setPlaceholderText(tr("Filter symbols..."));
    QList<FuzzyPickerItem> items;
    int index = 0;
    for (const LspSymbol &sym : symbols) {
        FuzzyPickerItem item;
        item.id = QString::number(index++);
        item.display = sym.name;
        const QString path = lspUriToPath(sym.uri);
        item.hint = QFileInfo(path.isEmpty() && m_editors ? m_editors->currentFilePath() : path).fileName()
            + QStringLiteral(":") + QString::number(sym.selectionRange.start.line + 1);
        item.filterText = sym.name + QLatin1Char(' ') + path;
        items.append(item);
    }
    dialog.setItems(items);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    const int chosen = dialog.selectedId().toInt();
    if (chosen < 0 || chosen >= symbols.size()) {
        return;
    }
    const LspSymbol &sym = symbols.at(chosen);
    QString path = lspUriToPath(sym.uri);
    if (path.isEmpty() && m_editors) {
        path = m_editors->currentFilePath();
    }
    m_editors->revealLocation(path, sym.selectionRange.start.line, sym.selectionRange.start.character);
}
