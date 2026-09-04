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
#include "lsp/LspDiagnosticsProvider.h"
#include "lsp/LspDocumentSync.h"
#include "lsp/LspServerManager.h"
#include "lsp/LspTypes.h"
#include "syntax/LanguageRegistry.h"

#include <QFileInfo>
#include <QTextCursor>
#include <QTimer>

namespace {

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
    connect(editor, &CodeEditor::hoverCanceled, this, &LspSession::dismissHover);
    if (auto *doc = EditorDocument::fromEditor(editor)) {
        connect(doc, &EditorDocument::versionChanged, this, [this, editor](int) {
            onEditorContentsChanged(editor);
        });
    }
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
    emit problemsChanged(path, {});
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
    if (m_completeWhenReady) {
        m_completeWhenReady = false;
        triggerCompletion();
    }
}

void LspSession::onDiagnostics(const QString &uri, const QVector<LspDiagnostic> &diagnostics)
{
    const QString path = lspUriToPath(uri);
    QVector<EditorDiagnostic> mapped;
    QVector<ProblemItem> problems;
    mapped.reserve(diagnostics.size());
    problems.reserve(diagnostics.size());
    for (const LspDiagnostic &diag : diagnostics) {
        const EditorDiagnostic editorDiag = toEditorDiagnostic(diag);
        mapped.append(editorDiag);
        ProblemItem problem;
        problem.path = path;
        problem.line = diag.range.start.line;
        problem.column = diag.range.start.character;
        problem.severity = editorDiag.severity;
        problem.message = diag.message;
        problem.source = diag.source;
        problem.code = diag.code;
        problems.append(problem);
    }
    if (!path.isEmpty()) {
        emit problemsChanged(path, problems);
    }
    if (!m_editors) {
        return;
    }
    if (CodeEditor *editor = m_editors->editorForPath(path)) {
        editor->setDiagnostics(mapped);
    }
}

void LspSession::onEditorContentsChanged(CodeEditor *editor)
{
    m_pendingChangeEditor = editor;
    m_changeTimer->start();
    if (m_completion->isVisibleFor(editor)) {
        m_completion->updateFilter(completionPrefixAtCursor(editor->textCursor()));
    }
}

void LspSession::dismissHover()
{
    ++m_hoverGeneration;
    if (m_hover) {
        m_hover->hidePopup();
    }
}

void LspSession::flushPendingChange()
{
    m_changeTimer->stop();
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
