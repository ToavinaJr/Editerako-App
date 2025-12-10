#include "codeeditor.h"
#include <QPainter>
#include <QTextBlock>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QTextCursor>
#include <algorithm>
#include <QVector>

CodeEditor::CodeEditor(QWidget *parent) : QPlainTextEdit(parent), lineNumbersVisible(true)
{
    lineNumberArea = new LineNumberArea(this);

    connect(this, &CodeEditor::blockCountChanged, this, &CodeEditor::updateLineNumberAreaWidth);
    connect(this, &CodeEditor::updateRequest, this, &CodeEditor::updateLineNumberArea);
    connect(this, &CodeEditor::cursorPositionChanged, this, &CodeEditor::highlightCurrentLine);

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
}

int CodeEditor::lineNumberAreaWidth()
{
    if (!lineNumbersVisible) {
        return 0;
    }

    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }

    int space = 3 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
    return space;
}

void CodeEditor::updateLineNumberAreaWidth(int /* newBlockCount */)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CodeEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy)
        lineNumberArea->scroll(0, dy);
    else
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void CodeEditor::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);

    QRect cr = contentsRect();
    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void CodeEditor::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;

        QColor lineColor = QColor(Qt::yellow).lighter(160);
        lineColor.setAlpha(30);

        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    setExtraSelections(extraSelections);
}

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    if (!lineNumbersVisible) {
        return;
    }

    QPainter painter(lineNumberArea);
    painter.fillRect(event->rect(), QColor(45, 45, 48)); 

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            painter.setPen(QColor(128, 128, 128)); 
            painter.drawText(0, top, lineNumberArea->width() - 3, fontMetrics().height(),
                             Qt::AlignRight, number);
        }

        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

void CodeEditor::setLineNumbersVisible(bool visible)
{
    if (lineNumbersVisible != visible) {
        lineNumbersVisible = visible;
        lineNumberArea->setVisible(visible);
        updateLineNumberAreaWidth(0);
        lineNumberArea->update();
    }
}

// Normalize extra cursors: remove duplicates and any that equal the primary cursor, sort ascending
void CodeEditor::normalizeExtraCursors()
{
    QTextCursor primary = textCursor();
    QVector<int> seen;
    QList<QTextCursor> out;
    for (const QTextCursor &c : extraCursors) {
        int pos = c.position();
        if (pos == primary.position()) continue;
        if (!std::binary_search(seen.begin(), seen.end(), pos)) {
            out.append(c);
            seen.append(pos);
            std::sort(seen.begin(), seen.end());
        }
    }
    // sort out by position
    std::sort(out.begin(), out.end(), [](const QTextCursor &a, const QTextCursor &b){ return a.position() < b.position(); });
    extraCursors = out;
}

void CodeEditor::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && (event->modifiers() & Qt::ControlModifier)) {
        // Ctrl+Click: toggle extra cursor at clicked position
        QTextCursor clicked = cursorForPosition(event->pos());
        int pos = clicked.position();
        bool removed = false;
        for (int i = 0; i < extraCursors.size(); ++i) {
            if (extraCursors[i].position() == pos) {
                extraCursors.removeAt(i);
                removed = true;
                break;
            }
        }
        if (!removed) {
            extraCursors.append(clicked);
        }
        normalizeExtraCursors();
        viewport()->update();
        return; // consume
    }

    // Click without Ctrl: clear all extra cursors and revert to single cursor
    if (!extraCursors.isEmpty()) {
        extraCursors.clear();
        viewport()->update();
    }

    QPlainTextEdit::mousePressEvent(event);
}

void CodeEditor::paintEvent(QPaintEvent *event)
{
    QPlainTextEdit::paintEvent(event);

    if (extraCursors.isEmpty()) return;

    QPainter painter(viewport());
    QColor caretColor = QColor(150, 150, 150, 220);
    for (const QTextCursor &c : extraCursors) {
        QRect r = cursorRect(c);
        QRect caretRect(r.left(), r.top(), qMax(2, r.width()/8), r.height());
        painter.fillRect(caretRect, caretColor);
        if (c.hasSelection()) {
            QTextCursor sel = c;
            QRect r1 = cursorRect(sel);
            painter.fillRect(r1, QColor(100,100,180,60));
        }
    }
}

void CodeEditor::insertTextAtCursors(const QString &text)
{
    if (text.isEmpty()) return;

    // Collect all cursor positions (extra + primary), sorted descending
    QList<int> positions;
    for (const QTextCursor &c : extraCursors) {
        positions.append(c.position());
    }
    positions.append(textCursor().position());
    std::sort(positions.begin(), positions.end(), std::greater<int>());

    int textLen = text.length();

    QTextCursor editBlock(document());
    editBlock.beginEditBlock();
    for (int pos : positions) {
        QTextCursor tc(document());
        tc.setPosition(pos);
        tc.insertText(text);
    }
    editBlock.endEditBlock();

    // Recalculate extra cursor positions after insertion
    // Sort ascending to compute new positions with cumulative offset
    std::sort(positions.begin(), positions.end());
    QList<QTextCursor> newExtras;
    int primaryPos = textCursor().position();
    int offset = 0;
    for (int origPos : positions) {
        int newPos = origPos + offset + textLen;
        offset += textLen;
        // Skip the primary cursor position (we update it via setTextCursor)
        if (origPos == primaryPos - textLen) continue; // primary was at this original pos
    }

    // Rebuild extra cursors at new positions
    extraCursors.clear();
    offset = 0;
    for (int i = 0; i < positions.size(); ++i) {
        int newPos = positions[i] + (i + 1) * textLen;
        // Check if this was the primary cursor original position
        // We skip the last one (highest original pos) since that becomes the new primary
        if (i < positions.size() - 1) {
            QTextCursor nc(document());
            nc.setPosition(newPos);
            extraCursors.append(nc);
        }
    }

    viewport()->update();
}

void CodeEditor::deleteAtCursors(bool backspace)
{
    // Collect all cursor positions (extra + primary), sorted descending
    QList<int> positions;
    for (const QTextCursor &c : extraCursors) {
        positions.append(c.position());
    }
    positions.append(textCursor().position());
    std::sort(positions.begin(), positions.end(), std::greater<int>());

    QTextCursor editBlock(document());
    editBlock.beginEditBlock();
    for (int pos : positions) {
        QTextCursor tc(document());
        tc.setPosition(pos);
        if (backspace) tc.deletePreviousChar();
        else tc.deleteChar();
    }
    editBlock.endEditBlock();

    // Recalculate extra cursor positions after deletion
    // Each deletion shifts subsequent cursors by 1
    std::sort(positions.begin(), positions.end());
    extraCursors.clear();
    for (int i = 0; i < positions.size() - 1; ++i) {
        // Each cursor before the i-th one has been deleted, shifting this one left
        int newPos = positions[i] - i - (backspace ? 1 : 0);
        if (newPos < 0) newPos = 0;
        QTextCursor nc(document());
        nc.setPosition(newPos);
        extraCursors.append(nc);
    }

    viewport()->update();
}

void CodeEditor::swapLineUp()
{
    QTextCursor primary = textCursor();
    QTextBlock cur = primary.block();
    if (!cur.isValid()) return;
    QTextBlock prev = cur.previous();
    if (!prev.isValid()) return;

    int startPrev = prev.position();
    int lenPrev = prev.length();
    int startCur = cur.position();
    int lenCur = cur.length();

    QString prevText = document()->toPlainText().mid(startPrev, lenPrev);
    QString curText = document()->toPlainText().mid(startCur, lenCur);

    QTextCursor editBlock(document());
    editBlock.beginEditBlock();
    QTextCursor c2(document());
    c2.setPosition(startCur);
    c2.setPosition(startCur + lenCur, QTextCursor::KeepAnchor);
    c2.removeSelectedText();
    c2.insertText(prevText);

    QTextCursor c1(document());
    c1.setPosition(startPrev);
    c1.setPosition(startPrev + lenPrev, QTextCursor::KeepAnchor);
    c1.removeSelectedText();
    c1.insertText(curText);
    editBlock.endEditBlock();

    QTextCursor newCursor(document());
    newCursor.setPosition(startPrev);
    setTextCursor(newCursor);
}

void CodeEditor::swapLineDown()
{
    QTextCursor primary = textCursor();
    QTextBlock cur = primary.block();
    QTextBlock next = cur.next();
    if (!next.isValid()) return;

    int startCur = cur.position();
    int lenCur = cur.length();
    int startNext = next.position();
    int lenNext = next.length();

    QString curText = document()->toPlainText().mid(startCur, lenCur);
    QString nextText = document()->toPlainText().mid(startNext, lenNext);

    QTextCursor editBlock(document());
    editBlock.beginEditBlock();
    QTextCursor cNext(document());
    cNext.setPosition(startNext);
    cNext.setPosition(startNext + lenNext, QTextCursor::KeepAnchor);
    cNext.removeSelectedText();
    cNext.insertText(curText);

    QTextCursor cCur(document());
    cCur.setPosition(startCur);
    cCur.setPosition(startCur + lenCur, QTextCursor::KeepAnchor);
    cCur.removeSelectedText();
    cCur.insertText(nextText);
    editBlock.endEditBlock();

    QTextCursor newCursor(document());
    newCursor.setPosition(startNext);
    setTextCursor(newCursor);
}

void CodeEditor::keyPressEvent(QKeyEvent *event)
{
    // Ctrl+Up / Ctrl+Down for swapping lines
    if (event->modifiers() & Qt::ControlModifier) {
        if (event->key() == Qt::Key_Up) { swapLineUp(); return; }
        if (event->key() == Qt::Key_Down) { swapLineDown(); return; }
    }

    // If no extra cursors, default behaviour
    if (extraCursors.isEmpty()) {
        QPlainTextEdit::keyPressEvent(event);
        return;
    }

    // Handle simple text insertion
    if (!event->text().isEmpty() && event->text().at(0).isPrint()) {
        insertTextAtCursors(event->text());
        return;
    }

    if (event->key() == Qt::Key_Backspace) { deleteAtCursors(true); return; }
    if (event->key() == Qt::Key_Delete) { deleteAtCursors(false); return; }

    // For other keys (arrows, etc.), just pass through without clearing cursors
    QPlainTextEdit::keyPressEvent(event);
    viewport()->update();
}

bool CodeEditor::isLineNumbersVisible() const
{
    return lineNumbersVisible;
}
