#include "editor/features/CommentController.h"

#include "editor/features/CommentOps.h"
#include "editor/features/LineBlockOps.h"

#include <QPlainTextEdit>
#include <QTextCursor>

CommentController::CommentController(QPlainTextEdit *editor)
    : m_editor(editor)
{
}

void CommentController::toggleLineComment(const QString &lineMarker)
{
    if (!m_editor || lineMarker.isEmpty()) {
        return;
    }
    const LineRange range = selectedLineRange(m_editor);
    replaceSelectedLines(m_editor, range, toggleLineComments(selectedLines(m_editor, range), lineMarker));
}

void CommentController::toggleBlockComment(const QString &open, const QString &close)
{
    if (!m_editor || open.isEmpty() || close.isEmpty()) {
        return;
    }
    QTextCursor cursor = m_editor->textCursor();
    if (!cursor.hasSelection()) {
        cursor.select(QTextCursor::WordUnderCursor);
    }
    const QString selected = cursor.selectedText().replace(QChar::ParagraphSeparator, QLatin1Char('\n'));
    cursor.insertText(::toggleBlockComment(selected, open, close));
}
