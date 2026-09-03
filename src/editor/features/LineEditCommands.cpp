#include "editor/features/LineEditCommands.h"

#include "editor/features/IndentOps.h"
#include "editor/features/LineBlockOps.h"

#include <QPlainTextEdit>
#include <QTextBlock>
#include <QTextCursor>
#include <QtGlobal>

LineEditCommands::LineEditCommands(QPlainTextEdit *editor)
    : m_editor(editor)
{
}

void LineEditCommands::duplicateLine()
{
    if (!m_editor) {
        return;
    }
    const LineRange range = selectedLineRange(m_editor);
    const QStringList lines = selectedLines(m_editor, range);
    QTextBlock last = m_editor->document()->findBlockByNumber(range.lastBlock);
    if (!last.isValid()) {
        return;
    }
    QTextCursor cursor(m_editor->document());
    cursor.beginEditBlock();
    cursor.setPosition(last.position() + last.text().size());
    cursor.insertText(QLatin1Char('\n') + lines.join(QLatin1Char('\n')));
    cursor.endEditBlock();
}

void LineEditCommands::deleteLine()
{
    if (!m_editor) {
        return;
    }
    const LineRange range = selectedLineRange(m_editor);
    QTextBlock first = m_editor->document()->findBlockByNumber(range.firstBlock);
    QTextBlock last = m_editor->document()->findBlockByNumber(range.lastBlock);
    if (!first.isValid() || !last.isValid()) {
        return;
    }
    QTextCursor cursor(m_editor->document());
    cursor.beginEditBlock();
    cursor.setPosition(first.position());
    if (last.next().isValid()) {
        cursor.setPosition(last.next().position(), QTextCursor::KeepAnchor);
    } else if (first.previous().isValid()) {
        cursor.setPosition(first.previous().position() + first.previous().text().size());
        cursor.setPosition(last.position() + last.text().size(), QTextCursor::KeepAnchor);
    } else {
        cursor.select(QTextCursor::Document);
    }
    cursor.removeSelectedText();
    cursor.endEditBlock();
}

void LineEditCommands::selectLine()
{
    if (!m_editor) {
        return;
    }
    QTextCursor cursor = m_editor->textCursor();
    cursor.movePosition(QTextCursor::StartOfBlock);
    cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    if (cursor.block().next().isValid()) {
        cursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor);
    }
    m_editor->setTextCursor(cursor);
}

void LineEditCommands::joinLines()
{
    if (!m_editor) {
        return;
    }
    LineRange range = selectedLineRange(m_editor);
    if (range.firstBlock == range.lastBlock) {
        range.lastBlock = qMin(range.firstBlock + 1, m_editor->document()->blockCount() - 1);
    }
    if (range.firstBlock == range.lastBlock) {
        return;
    }
    QStringList lines = selectedLines(m_editor, range);
    for (QString &line : lines) {
        line = line.trimmed();
    }
    replaceSelectedLines(m_editor, range, {lines.join(QLatin1Char(' '))});
}

void LineEditCommands::sortLines()
{
    if (!m_editor) {
        return;
    }
    const LineRange range = selectedLineRange(m_editor);
    QStringList lines = selectedLines(m_editor, range);
    if (lines.size() < 2) {
        return;
    }
    replaceSelectedLines(m_editor, range, sortLinesText(lines.join(QLatin1Char('\n'))).split(QLatin1Char('\n')));
}

void LineEditCommands::trimTrailingWhitespace()
{
    if (!m_editor) {
        return;
    }
    replaceDocumentText(m_editor, ::trimTrailingWhitespace(m_editor->toPlainText()));
}
