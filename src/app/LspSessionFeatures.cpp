#include "app/LspSession.h"

#include "editor/CodeEditor.h"
#include "editor/CompletionModel.h"
#include "editor/CompletionPopup.h"
#include "editor/EditorDocument.h"
#include "editor/EditorManager.h"
#include "editor/HoverPopup.h"
#include "lsp/LspCompletionProvider.h"
#include "lsp/LspHoverProvider.h"
#include "lsp/LspServerManager.h"
#include "lsp/LspTypes.h"

#include <QTextCursor>

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

} // namespace

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
