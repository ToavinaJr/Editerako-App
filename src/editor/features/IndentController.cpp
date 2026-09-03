#include "editor/features/IndentController.h"

#include "core/AppSettings.h"
#include "editor/features/IndentOps.h"
#include "editor/features/LineBlockOps.h"

#include <QKeyEvent>
#include <QPlainTextEdit>
#include <QTextBlock>
#include <QTextCursor>

IndentController::IndentController(QPlainTextEdit *editor)
    : m_editor(editor)
{
}

void IndentController::indent()
{
    if (!m_editor) {
        return;
    }
    const AppSettings settings;
    LineRange range = selectedLineRange(m_editor);
    QStringList lines = selectedLines(m_editor, range);
    indentLines(&lines, settings.editorTabSize(), settings.editorInsertSpaces());
    replaceSelectedLines(m_editor, range, lines);
}

void IndentController::outdent()
{
    if (!m_editor) {
        return;
    }
    const AppSettings settings;
    LineRange range = selectedLineRange(m_editor);
    QStringList lines = selectedLines(m_editor, range);
    outdentLines(&lines, settings.editorTabSize(), settings.editorInsertSpaces());
    replaceSelectedLines(m_editor, range, lines);
}

void IndentController::convertToSpaces()
{
    if (!m_editor) {
        return;
    }
    const AppSettings settings;
    const QString updated = convertIndentation(m_editor->toPlainText(), true, settings.editorTabSize());
    replaceDocumentText(m_editor, updated);
}

void IndentController::convertToTabs()
{
    if (!m_editor) {
        return;
    }
    const AppSettings settings;
    const QString updated = convertIndentation(m_editor->toPlainText(), false, settings.editorTabSize());
    replaceDocumentText(m_editor, updated);
}

bool IndentController::handleKeyPress(QKeyEvent *event)
{
    if (!m_editor || !event) {
        return false;
    }

    if (event->key() == Qt::Key_Backtab
        || (event->key() == Qt::Key_Tab && event->modifiers() & Qt::ShiftModifier)) {
        outdent();
        return true;
    }

    if (event->key() == Qt::Key_Tab && event->modifiers() == Qt::NoModifier) {
        if (m_editor->textCursor().hasSelection()) {
            indent();
            return true;
        }
        const AppSettings settings;
        if (settings.editorInsertSpaces()) {
            m_editor->insertPlainText(indentUnit(settings.editorTabSize(), true));
            return true;
        }
        return false;
    }

    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if (event->modifiers() != Qt::NoModifier && event->modifiers() != Qt::KeypadModifier) {
            return false;
        }
        const AppSettings settings;
        const QString line = m_editor->textCursor().block().text();
        const QString prefix = smartIndentPrefix(line, settings.editorTabSize(), settings.editorInsertSpaces());
        QTextCursor cursor = m_editor->textCursor();
        cursor.beginEditBlock();
        cursor.insertText(QLatin1Char('\n') + prefix);
        cursor.endEditBlock();
        m_editor->setTextCursor(cursor);
        return true;
    }
    return false;
}
