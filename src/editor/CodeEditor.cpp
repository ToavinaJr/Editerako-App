#include "editor/CodeEditor.h"

#include "core/AppSettings.h"
#include "editor/EditorDocument.h"
#include "editor/features/AutoClosingPairs.h"
#include "editor/features/BracketMatcher.h"
#include "editor/features/CurrentLineHighlighter.h"
#include "editor/features/IndentOps.h"
#include "editor/features/LineNumberArea.h"
#include "syntax/LanguageRegistry.h"

#include <QColor>
#include <QKeyEvent>
#include <QList>
#include <QMouseEvent>
#include <QPainter>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextEdit>

CodeEditor::CodeEditor(QWidget *parent)
    : QPlainTextEdit(parent)
    , m_lineNumberArea(new LineNumberArea(this))
    , m_lineNumbersVisible(true)
    , m_multiCursor(this)
    , m_lineMovement(this)
    , m_indent(this)
    , m_comments(this)
    , m_lineEdits(this)
    , m_occurrences(this, &m_multiCursor)
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
    QList<QTextEdit::ExtraSelection> extras;
    const BracketMatch match = findBracketMatch(toPlainText(), textCursor().position());
    if (match.isValid()) {
        QTextEdit::ExtraSelection open;
        open.format.setBackground(QColor(120, 120, 200, 90));
        open.cursor = textCursor();
        open.cursor.setPosition(match.open);
        open.cursor.setPosition(match.open + 1, QTextCursor::KeepAnchor);
        extras.append(open);

        QTextEdit::ExtraSelection closeSel = open;
        closeSel.cursor.setPosition(match.close);
        closeSel.cursor.setPosition(match.close + 1, QTextCursor::KeepAnchor);
        extras.append(closeSel);
    }
    CurrentLineHighlighter::apply(this, extras);
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
    if (event->key() == Qt::Key_Tab && event->modifiers() == Qt::NoModifier
        && !textCursor().hasSelection() && !m_multiCursor.isEmpty()
        && AppSettings().editorInsertSpaces()) {
        m_multiCursor.insertText(indentUnit(AppSettings().editorTabSize(), true));
        return;
    }

    if (m_indent.handleKeyPress(event)) {
        return;
    }

    if (m_multiCursor.isEmpty() && !event->text().isEmpty()) {
        const QChar typed = event->text().at(0);
        QTextCursor cursor = textCursor();
        const QString text = toPlainText();
        const int pos = cursor.position();
        if (shouldSkipClosingPair(text, pos, typed)) {
            cursor.movePosition(QTextCursor::NextCharacter);
            setTextCursor(cursor);
            return;
        }
        const QChar closer = closingPairFor(typed);
        if (!closer.isNull() && cursor.hasSelection()) {
            const QString selected = cursor.selectedText();
            cursor.insertText(typed + selected + closer);
            return;
        }
        if (shouldInsertClosingPair(text, pos, typed)) {
            cursor.insertText(QString(typed) + closer);
            cursor.movePosition(QTextCursor::PreviousCharacter);
            setTextCursor(cursor);
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

void CodeEditor::indentSelection()
{
    m_indent.indent();
}

void CodeEditor::outdentSelection()
{
    m_indent.outdent();
}

void CodeEditor::toggleLineComment()
{
    const auto *doc = EditorDocument::fromEditor(this);
    const CommentTokens tokens = LanguageRegistry::commentTokens(
        doc ? doc->language() : LanguageId::PlainText);
    m_comments.toggleLineComment(tokens.line);
}

void CodeEditor::toggleBlockComment()
{
    const auto *doc = EditorDocument::fromEditor(this);
    const CommentTokens tokens = LanguageRegistry::commentTokens(
        doc ? doc->language() : LanguageId::PlainText);
    m_comments.toggleBlockComment(tokens.blockOpen, tokens.blockClose);
}

void CodeEditor::duplicateLine()
{
    m_lineEdits.duplicateLine();
}

void CodeEditor::deleteLine()
{
    m_lineEdits.deleteLine();
}

void CodeEditor::moveLineUp()
{
    m_lineMovement.moveUp();
}

void CodeEditor::moveLineDown()
{
    m_lineMovement.moveDown();
}

void CodeEditor::selectLine()
{
    m_lineEdits.selectLine();
}

void CodeEditor::joinLines()
{
    m_lineEdits.joinLines();
}

void CodeEditor::sortSelectedLines()
{
    m_lineEdits.sortLines();
}

void CodeEditor::trimTrailingWhitespace()
{
    m_lineEdits.trimTrailingWhitespace();
}

void CodeEditor::convertIndentationToSpaces()
{
    m_indent.convertToSpaces();
}

void CodeEditor::convertIndentationToTabs()
{
    m_indent.convertToTabs();
}

void CodeEditor::selectNextOccurrence()
{
    m_occurrences.selectNext();
}

void CodeEditor::selectAllOccurrences()
{
    m_occurrences.selectAll();
}
