#include "editor/features/LineMovementController.h"

#include <QPlainTextEdit>
#include <QTextBlock>
#include <QTextCursor>

LineMovementController::LineMovementController(QPlainTextEdit *editor)
    : m_editor(editor)
{
}

void LineMovementController::moveUp()
{
    const QTextCursor primary = m_editor->textCursor();
    const QTextBlock cur = primary.block();
    if (!cur.isValid()) {
        return;
    }
    const QTextBlock prev = cur.previous();
    if (!prev.isValid()) {
        return;
    }

    const int startPrev = prev.position();
    const int lenPrev = prev.length();
    const int startCur = cur.position();
    const int lenCur = cur.length();

    const QString prevText = m_editor->document()->toPlainText().mid(startPrev, lenPrev);
    const QString curText = m_editor->document()->toPlainText().mid(startCur, lenCur);

    QTextCursor editBlock(m_editor->document());
    editBlock.beginEditBlock();
    QTextCursor c2(m_editor->document());
    c2.setPosition(startCur);
    c2.setPosition(startCur + lenCur, QTextCursor::KeepAnchor);
    c2.removeSelectedText();
    c2.insertText(prevText);

    QTextCursor c1(m_editor->document());
    c1.setPosition(startPrev);
    c1.setPosition(startPrev + lenPrev, QTextCursor::KeepAnchor);
    c1.removeSelectedText();
    c1.insertText(curText);
    editBlock.endEditBlock();

    QTextCursor newCursor(m_editor->document());
    newCursor.setPosition(startPrev);
    m_editor->setTextCursor(newCursor);
}

void LineMovementController::moveDown()
{
    const QTextCursor primary = m_editor->textCursor();
    const QTextBlock cur = primary.block();
    const QTextBlock next = cur.next();
    if (!next.isValid()) {
        return;
    }

    const int startCur = cur.position();
    const int lenCur = cur.length();
    const int startNext = next.position();
    const int lenNext = next.length();

    const QString curText = m_editor->document()->toPlainText().mid(startCur, lenCur);
    const QString nextText = m_editor->document()->toPlainText().mid(startNext, lenNext);

    QTextCursor editBlock(m_editor->document());
    editBlock.beginEditBlock();
    QTextCursor cNext(m_editor->document());
    cNext.setPosition(startNext);
    cNext.setPosition(startNext + lenNext, QTextCursor::KeepAnchor);
    cNext.removeSelectedText();
    cNext.insertText(curText);

    QTextCursor cCur(m_editor->document());
    cCur.setPosition(startCur);
    cCur.setPosition(startCur + lenCur, QTextCursor::KeepAnchor);
    cCur.removeSelectedText();
    cCur.insertText(nextText);
    editBlock.endEditBlock();

    QTextCursor newCursor(m_editor->document());
    newCursor.setPosition(startNext);
    m_editor->setTextCursor(newCursor);
}
