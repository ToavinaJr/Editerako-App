#include "editor/features/MultiCursorController.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPlainTextEdit>
#include <QVector>
#include <algorithm>
#include <functional>

MultiCursorController::MultiCursorController(QPlainTextEdit *editor)
    : m_editor(editor)
{
}

void MultiCursorController::normalize()
{
    const QTextCursor primary = m_editor->textCursor();
    QVector<int> seen;
    QList<QTextCursor> out;
    for (const QTextCursor &c : m_extraCursors) {
        const int pos = c.position();
        if (pos == primary.position()) {
            continue;
        }
        if (!std::binary_search(seen.begin(), seen.end(), pos)) {
            out.append(c);
            seen.append(pos);
            std::sort(seen.begin(), seen.end());
        }
    }
    std::sort(out.begin(), out.end(), [](const QTextCursor &a, const QTextCursor &b) {
        return a.position() < b.position();
    });
    m_extraCursors = out;
}

void MultiCursorController::clear()
{
    m_extraCursors.clear();
}

void MultiCursorController::toggleAt(const QTextCursor &clicked)
{
    const int pos = clicked.position();
    bool removed = false;
    for (int i = 0; i < m_extraCursors.size(); ++i) {
        if (m_extraCursors[i].position() == pos) {
            m_extraCursors.removeAt(i);
            removed = true;
            break;
        }
    }
    if (!removed) {
        m_extraCursors.append(clicked);
    }
    normalize();
    m_editor->viewport()->update();
}

void MultiCursorController::addExtra(const QTextCursor &cursor)
{
    m_extraCursors.append(cursor);
    normalize();
    m_editor->viewport()->update();
}

void MultiCursorController::setExtras(const QList<QTextCursor> &cursors)
{
    m_extraCursors = cursors;
    normalize();
    m_editor->viewport()->update();
}

bool MultiCursorController::handleMousePress(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && (event->modifiers() & Qt::AltModifier)) {
        toggleAt(m_editor->cursorForPosition(event->pos()));
        return true;
    }

    if (!m_extraCursors.isEmpty()) {
        clear();
        m_editor->viewport()->update();
    }
    return false;
}

void MultiCursorController::paint(QPaintEvent *event) const
{
    Q_UNUSED(event)
    if (m_extraCursors.isEmpty()) {
        return;
    }

    QPainter painter(m_editor->viewport());
    const QColor caretColor(150, 150, 150, 220);
    for (const QTextCursor &c : m_extraCursors) {
        const QRect r = m_editor->cursorRect(c);
        const QRect caretRect(r.left(), r.top(), qMax(2, r.width() / 8), r.height());
        painter.fillRect(caretRect, caretColor);
        if (c.hasSelection()) {
            painter.fillRect(m_editor->cursorRect(c), QColor(100, 100, 180, 60));
        }
    }
}

void MultiCursorController::insertText(const QString &text)
{
    if (text.isEmpty()) {
        return;
    }

    QList<int> positions;
    for (const QTextCursor &c : m_extraCursors) {
        positions.append(c.position());
    }
    positions.append(m_editor->textCursor().position());
    std::sort(positions.begin(), positions.end(), std::greater<int>());

    const int textLen = text.length();

    QTextCursor editBlock(m_editor->document());
    editBlock.beginEditBlock();
    for (int pos : positions) {
        QTextCursor tc(m_editor->document());
        tc.setPosition(pos);
        tc.insertText(text);
    }
    editBlock.endEditBlock();

    std::sort(positions.begin(), positions.end());
    m_extraCursors.clear();
    for (int i = 0; i < positions.size(); ++i) {
        const int newPos = positions[i] + (i + 1) * textLen;
        if (i < positions.size() - 1) {
            QTextCursor nc(m_editor->document());
            nc.setPosition(newPos);
            m_extraCursors.append(nc);
        }
    }

    m_editor->viewport()->update();
}

void MultiCursorController::deleteAtCursors(bool backspace)
{
    QList<int> positions;
    for (const QTextCursor &c : m_extraCursors) {
        positions.append(c.position());
    }
    positions.append(m_editor->textCursor().position());
    std::sort(positions.begin(), positions.end(), std::greater<int>());

    QTextCursor editBlock(m_editor->document());
    editBlock.beginEditBlock();
    for (int pos : positions) {
        QTextCursor tc(m_editor->document());
        tc.setPosition(pos);
        if (backspace) {
            tc.deletePreviousChar();
        } else {
            tc.deleteChar();
        }
    }
    editBlock.endEditBlock();

    std::sort(positions.begin(), positions.end());
    m_extraCursors.clear();
    for (int i = 0; i < positions.size() - 1; ++i) {
        int newPos = positions[i] - i - (backspace ? 1 : 0);
        if (newPos < 0) {
            newPos = 0;
        }
        QTextCursor nc(m_editor->document());
        nc.setPosition(newPos);
        m_extraCursors.append(nc);
    }

    m_editor->viewport()->update();
}

bool MultiCursorController::handleKeyPress(QKeyEvent *event)
{
    if (m_extraCursors.isEmpty()) {
        return false;
    }

    if (!event->text().isEmpty() && event->text().at(0).isPrint()) {
        insertText(event->text());
        return true;
    }
    if (event->key() == Qt::Key_Backspace) {
        deleteAtCursors(true);
        return true;
    }
    if (event->key() == Qt::Key_Delete) {
        deleteAtCursors(false);
        return true;
    }
    return false;
}
