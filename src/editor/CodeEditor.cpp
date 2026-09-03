#include "editor/CodeEditor.h"

#include "editor/features/CurrentLineHighlighter.h"
#include "editor/features/LineNumberArea.h"

#include <QColor>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QTextBlock>

CodeEditor::CodeEditor(QWidget *parent)
    : QPlainTextEdit(parent)
    , m_lineNumberArea(new LineNumberArea(this))
    , m_lineNumbersVisible(true)
    , m_multiCursor(this)
    , m_lineMovement(this)
{
    connect(this, &CodeEditor::blockCountChanged, this, &CodeEditor::updateLineNumberAreaWidth);
    connect(this, &CodeEditor::updateRequest, this, &CodeEditor::updateLineNumberArea);
    connect(this, &CodeEditor::cursorPositionChanged, this, &CodeEditor::highlightCurrentLine);

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
}

int CodeEditor::lineNumberAreaWidth() const
{
    if (!m_lineNumbersVisible) {
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
    if (dy) {
        m_lineNumberArea->scroll(0, dy);
    } else {
        m_lineNumberArea->update(0, rect.y(), m_lineNumberArea->width(), rect.height());
    }

    if (rect.contains(viewport()->rect())) {
        updateLineNumberAreaWidth(0);
    }
}

void CodeEditor::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);

    const QRect cr = contentsRect();
    m_lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void CodeEditor::highlightCurrentLine()
{
    CurrentLineHighlighter::apply(this);
}

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    if (!m_lineNumbersVisible) {
        return;
    }

    QPainter painter(m_lineNumberArea);
    painter.fillRect(event->rect(), QColor(45, 45, 48));

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            const QString number = QString::number(blockNumber + 1);
            painter.setPen(QColor(128, 128, 128));
            painter.drawText(0, top, m_lineNumberArea->width() - 3, fontMetrics().height(),
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
    if (m_lineNumbersVisible != visible) {
        m_lineNumbersVisible = visible;
        m_lineNumberArea->setVisible(visible);
        updateLineNumberAreaWidth(0);
        m_lineNumberArea->update();
    }
}

bool CodeEditor::isLineNumbersVisible() const
{
    return m_lineNumbersVisible;
}

void CodeEditor::mousePressEvent(QMouseEvent *event)
{
    if (m_multiCursor.handleMousePress(event)) {
        return;
    }
    QPlainTextEdit::mousePressEvent(event);
}

void CodeEditor::paintEvent(QPaintEvent *event)
{
    QPlainTextEdit::paintEvent(event);
    m_multiCursor.paint(event);
}

void CodeEditor::keyPressEvent(QKeyEvent *event)
{
    if (event->modifiers() & Qt::ControlModifier) {
        if (event->key() == Qt::Key_Up) {
            m_lineMovement.moveUp();
            return;
        }
        if (event->key() == Qt::Key_Down) {
            m_lineMovement.moveDown();
            return;
        }
    }

    if (m_multiCursor.handleKeyPress(event)) {
        return;
    }

    QPlainTextEdit::keyPressEvent(event);
    if (!m_multiCursor.isEmpty()) {
        viewport()->update();
    }
}
