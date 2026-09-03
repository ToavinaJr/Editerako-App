#include "editor/features/OccurrenceController.h"

#include "editor/features/MultiCursorController.h"

#include <QList>
#include <QPlainTextEdit>
#include <QTextCursor>
#include <QTextDocument>

OccurrenceController::OccurrenceController(QPlainTextEdit *editor, MultiCursorController *multi)
    : m_editor(editor)
    , m_multi(multi)
{
}

void OccurrenceController::selectNext()
{
    if (!m_editor || !m_multi) {
        return;
    }
    QTextCursor primary = m_editor->textCursor();
    if (!primary.hasSelection()) {
        primary.select(QTextCursor::WordUnderCursor);
        m_editor->setTextCursor(primary);
        return;
    }

    const QString needle = primary.selectedText().replace(QChar::ParagraphSeparator, QLatin1Char('\n'));
    if (needle.isEmpty()) {
        return;
    }

    QTextCursor found = m_editor->document()->find(needle, primary.selectionEnd(),
                                                   QTextDocument::FindCaseSensitively);
    if (found.isNull()) {
        found = m_editor->document()->find(needle, 0, QTextDocument::FindCaseSensitively);
    }
    if (found.isNull() || found.position() == primary.position()) {
        return;
    }
    // Move the primary first so addExtra() does not drop the previous
    // selection as a duplicate of the still-current cursor.
    m_editor->setTextCursor(found);
    m_multi->addExtra(primary);
}

void OccurrenceController::selectAll()
{
    if (!m_editor || !m_multi) {
        return;
    }
    QTextCursor primary = m_editor->textCursor();
    if (!primary.hasSelection()) {
        primary.select(QTextCursor::WordUnderCursor);
        m_editor->setTextCursor(primary);
    }
    const QString needle = primary.selectedText().replace(QChar::ParagraphSeparator, QLatin1Char('\n'));
    if (needle.isEmpty()) {
        return;
    }

    QList<QTextCursor> matches;
    QTextCursor found = m_editor->document()->find(needle, 0, QTextDocument::FindCaseSensitively);
    while (!found.isNull()) {
        matches.append(found);
        found = m_editor->document()->find(needle, found.selectionEnd(),
                                           QTextDocument::FindCaseSensitively);
    }
    if (matches.isEmpty()) {
        return;
    }
    m_editor->setTextCursor(matches.first());
    matches.removeFirst();
    m_multi->setExtras(matches);
}
