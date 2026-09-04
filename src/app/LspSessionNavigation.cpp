#include "app/LspSession.h"

#include "editor/CodeEditor.h"
#include "editor/EditorDiagnostic.h"
#include "editor/EditorDocument.h"
#include "editor/EditorManager.h"
#include "lsp/LspNavigationProvider.h"
#include "lsp/LspPickerItems.h"
#include "lsp/LspServerManager.h"
#include "lsp/LspSymbolProvider.h"
#include "lsp/LspTypes.h"
#include "ui/FuzzyPickerDialog.h"

#include <QInputDialog>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QTextCursor>
#include <QTextDocument>
#include <algorithm>

namespace {

QList<FuzzyPickerItem> pickerItemsFromRows(const QVector<LspPickerRow> &rows)
{
    QList<FuzzyPickerItem> items;
    items.reserve(rows.size());
    for (const LspPickerRow &row : rows) {
        FuzzyPickerItem item;
        item.id = row.id;
        item.display = row.display;
        item.hint = row.hint;
        item.filterText = row.filterText;
        items.append(item);
    }
    return items;
}

int execPicker(QWidget *parent, const QVector<LspPickerRow> &rows, const QString &title,
               const QString &placeholder)
{
    FuzzyPickerDialog dialog(parent);
    dialog.setWindowTitle(title);
    dialog.setPlaceholderText(placeholder);
    dialog.setItems(pickerItemsFromRows(rows));
    if (dialog.exec() != QDialog::Accepted) {
        return -1;
    }
    return dialog.selectedId().toInt();
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
    const QVector<LspPickerRow> rows = lspLocationRows(locations);
    if (rows.isEmpty()) {
        emit statusMessage(tr("No results"), 2000);
        return;
    }
    int chosen = 0;
    if (rows.size() > 1) {
        chosen = execPicker(m_dialogParent, rows, title, tr("Filter locations..."));
        if (chosen < 0) {
            return;
        }
    }
    if (chosen < 0 || chosen >= rows.size()) {
        return;
    }
    const LspPickerRow &row = rows.at(chosen);
    m_editors->revealLocation(row.path, row.line, row.character);
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
    const QString fallback = m_editors ? m_editors->currentFilePath() : QString();
    const QVector<LspPickerRow> rows = lspSymbolRows(symbols, fallback);
    if (rows.isEmpty()) {
        emit statusMessage(tr("No symbols"), 2000);
        return;
    }
    const int chosen = execPicker(m_dialogParent, rows, title, tr("Filter symbols..."));
    if (chosen < 0 || chosen >= rows.size()) {
        return;
    }
    const LspPickerRow &row = rows.at(chosen);
    m_editors->revealLocation(row.path, row.line, row.character);
}
