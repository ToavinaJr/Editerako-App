#include "editor/features/LineBlockOps.h"

#include <QPlainTextEdit>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>

LineRange selectedLineRange(QPlainTextEdit *editor)
{
    LineRange range;
    if (!editor) {
        return range;
    }
    const QTextCursor cursor = editor->textCursor();
    const int start = cursor.selectionStart();
    const int end = cursor.selectionEnd();
    QTextBlock first = editor->document()->findBlock(start);
    QTextBlock last = editor->document()->findBlock(end);
    if (cursor.hasSelection() && end == last.position() && last.blockNumber() > first.blockNumber()) {
        last = last.previous();
    }
    range.firstBlock = first.blockNumber();
    range.lastBlock = last.blockNumber();
    return range;
}

QStringList selectedLines(QPlainTextEdit *editor, const LineRange &range)
{
    QStringList lines;
    if (!editor) {
        return lines;
    }
    for (int i = range.firstBlock; i <= range.lastBlock; ++i) {
        const QTextBlock block = editor->document()->findBlockByNumber(i);
        if (block.isValid()) {
            lines.append(block.text());
        }
    }
    return lines;
}

void replaceSelectedLines(QPlainTextEdit *editor, const LineRange &range, const QStringList &lines)
{
    if (!editor || lines.isEmpty()) {
        return;
    }
    QTextDocument *doc = editor->document();
    const QTextBlock first = doc->findBlockByNumber(range.firstBlock);
    const QTextBlock last = doc->findBlockByNumber(range.lastBlock);
    if (!first.isValid() || !last.isValid()) {
        return;
    }

    QTextCursor cursor(doc);
    cursor.beginEditBlock();
    cursor.setPosition(first.position());
    cursor.setPosition(last.position() + last.text().size(), QTextCursor::KeepAnchor);
    cursor.insertText(lines.join(QLatin1Char('\n')));
    cursor.endEditBlock();
}

void replaceDocumentText(QPlainTextEdit *editor, const QString &text)
{
    if (!editor) {
        return;
    }
    QTextCursor cursor(editor->document());
    cursor.beginEditBlock();
    cursor.select(QTextCursor::Document);
    cursor.insertText(text);
    cursor.endEditBlock();
}
