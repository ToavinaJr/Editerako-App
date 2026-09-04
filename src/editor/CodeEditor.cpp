#include "editor/CodeEditor.h"

#include "core/AppSettings.h"
#include "editor/DiagnosticMarkup.h"
#include "editor/EditorDocument.h"
#include "editor/features/AutoClosingPairs.h"
#include "editor/features/BracketMatcher.h"
#include "editor/features/CurrentLineHighlighter.h"
#include "editor/features/IndentOps.h"
#include "editor/features/LineNumberArea.h"
#include "syntax/LanguageRegistry.h"

#include <QColor>
#include <QEvent>
#include <QHash>
#include <QKeyEvent>
#include <QList>
#include <QMouseEvent>
#include <QPainter>
#include <QPolygonF>
#include <QScrollBar>
#include <QWheelEvent>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextEdit>
#include <QTextFormat>
#include <QTimer>

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
    , m_folding(this)
    , m_hoverTimer(new QTimer(this))
    , m_foldTimer(new QTimer(this))
{
    connect(this, &CodeEditor::blockCountChanged, this, &CodeEditor::updateLineNumberAreaWidth);
    connect(this, &CodeEditor::updateRequest, this, &CodeEditor::updateLineNumberArea);
    connect(this, &CodeEditor::cursorPositionChanged, this, &CodeEditor::highlightCurrentLine);

    m_hoverTimer->setSingleShot(true);
    m_hoverTimer->setInterval(400);
    connect(m_hoverTimer, &QTimer::timeout, this, &CodeEditor::emitHover);
    connect(this, &CodeEditor::cursorPositionChanged, this, &CodeEditor::hoverCanceled);
    if (auto *bar = verticalScrollBar()) {
        connect(bar, &QScrollBar::valueChanged, this, &CodeEditor::hoverCanceled);
    }

    m_foldTimer->setSingleShot(true);
    m_foldTimer->setInterval(0);
    connect(m_foldTimer, &QTimer::timeout, this, [this]() {
        m_folding.refresh();
        if (m_lineNumberArea) {
            m_lineNumberArea->update();
        }
    });
    connect(document(), &QTextDocument::contentsChange, this, &CodeEditor::scheduleFoldRefresh);

    viewport()->setMouseTracking(true);
    setMouseTracking(true);

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

    int space = kBreakpointGutterWidth + kFoldGutterWidth + 3
        + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
    space += diagnosticGutterExtraWidth(m_diagnostics);
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
    QList<QTextEdit::ExtraSelection> extras = diagnosticExtraSelections(this, m_diagnostics);
    if (m_debugLine >= 0) {
        const QTextBlock debugBlock = document()->findBlockByNumber(m_debugLine);
        if (debugBlock.isValid()) {
            QTextEdit::ExtraSelection debugSel;
            debugSel.format.setBackground(QColor(200, 160, 40, 80));
            debugSel.format.setProperty(QTextFormat::FullWidthSelection, true);
            debugSel.cursor = QTextCursor(debugBlock);
            extras.append(debugSel);
        }
    }
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
    const QHash<int, EditorDiagnostic::Severity> worst = worstDiagnosticByLine(m_diagnostics);

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            if (blockNumber == m_debugLine) {
                painter.fillRect(0, top, m_lineNumberArea->width(), bottom - top,
                                 QColor(200, 160, 40, 70));
            }
            if (m_breakpointLines.contains(blockNumber)) {
                painter.setBrush(QColor(220, 50, 50));
                painter.setPen(Qt::NoPen);
                const int size = 8;
                const int y = top + qMax(0, (fontMetrics().height() - size) / 2);
                painter.drawEllipse(3, y, size, size);
            }
            if (m_folding.isFoldable(blockNumber)) {
                const int cy = top + fontMetrics().height() / 2;
                const int cx = kBreakpointGutterWidth + kFoldGutterWidth / 2;
                QPolygonF tri;
                if (m_folding.isFolded(blockNumber)) {
                    tri << QPointF(cx - 3, cy - 4) << QPointF(cx - 3, cy + 4)
                        << QPointF(cx + 4, cy);
                } else {
                    tri << QPointF(cx - 4, cy - 3) << QPointF(cx + 4, cy - 3)
                        << QPointF(cx, cy + 4);
                }
                painter.setBrush(QColor(160, 160, 160));
                painter.setPen(Qt::NoPen);
                painter.drawPolygon(tri);
            }
            const auto it = worst.constFind(block.blockNumber());
            if (it != worst.cend()) {
                painter.setBrush(diagnosticColor(it.value()));
                painter.setPen(Qt::NoPen);
                const int size = 6;
                const int y = top + qMax(0, (fontMetrics().height() - size) / 2);
                painter.drawEllipse(kBreakpointGutterWidth + kFoldGutterWidth, y, size, size);
            }
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
    }
    updateLineNumberAreaWidth(0);
    m_lineNumberArea->update();
}

bool CodeEditor::isLineNumbersVisible() const
{
    return m_lineNumbersVisible;
}

void CodeEditor::setBreakpointLines(const QSet<int> &lines)
{
    if (m_breakpointLines == lines) {
        return;
    }
    m_breakpointLines = lines;
    if (m_lineNumberArea) {
        m_lineNumberArea->update();
    }
}

void CodeEditor::setDebugLine(int line)
{
    if (m_debugLine == line) {
        return;
    }
    m_debugLine = line;
    highlightCurrentLine();
    if (m_lineNumberArea) {
        m_lineNumberArea->update();
    }
}

void CodeEditor::gutterMousePress(QMouseEvent *event)
{
    if (!event || event->button() != Qt::LeftButton || !m_lineNumbersVisible) {
        return;
    }

    QTextBlock block = firstVisibleBlock();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    while (block.isValid()) {
        const int bottom = top + qRound(blockBoundingRect(block).height());
        if (event->pos().y() >= top && event->pos().y() < bottom) {
            const int x = event->pos().x();
            if (x >= kBreakpointGutterWidth && x < kBreakpointGutterWidth + kFoldGutterWidth) {
                if (m_folding.isFoldable(block.blockNumber())) {
                    m_folding.toggleAt(block.blockNumber());
                    m_lineNumberArea->update();
                    event->accept();
                }
                return;
            }
            emit breakpointToggled(block.blockNumber());
            event->accept();
            return;
        }
        block = block.next();
        top = bottom;
    }
}

void CodeEditor::mousePressEvent(QMouseEvent *event)
{
    emit hoverCanceled();
    m_hoverTimer->stop();
    if (event->button() == Qt::LeftButton && (event->modifiers() & Qt::ControlModifier)
        && !(event->modifiers() & Qt::AltModifier)) {
        setTextCursor(cursorForPosition(event->pos()));
        emit definitionRequested();
        event->accept();
        return;
    }
    if (m_multiCursor.handleMousePress(event)) {
        return;
    }
    QPlainTextEdit::mousePressEvent(event);
}

void CodeEditor::mouseMoveEvent(QMouseEvent *event)
{
    QPlainTextEdit::mouseMoveEvent(event);
    viewport()->setCursor((event->modifiers() & Qt::ControlModifier) ? Qt::PointingHandCursor
                                                                     : Qt::IBeamCursor);
    const QTextCursor now = cursorForPosition(event->pos());
    const QTextCursor previous = cursorForPosition(m_hoverLocalPos);
    if (now.blockNumber() != previous.blockNumber()) {
        emit hoverCanceled();
    }
    m_hoverLocalPos = event->pos();
    m_hoverTimer->start();
}

void CodeEditor::leaveEvent(QEvent *event)
{
    m_hoverTimer->stop();
    viewport()->unsetCursor();
    QPlainTextEdit::leaveEvent(event);
}

void CodeEditor::emitHover()
{
    const QTextCursor cursor = cursorForPosition(m_hoverLocalPos);
    emit hoverRequested(cursor.blockNumber(), cursor.positionInBlock(),
                        mapToGlobal(m_hoverLocalPos));
}

void CodeEditor::setDiagnostics(const QVector<EditorDiagnostic> &diagnostics)
{
    m_diagnostics = diagnostics;
    updateLineNumberAreaWidth(0);
    m_lineNumberArea->update();
    highlightCurrentLine();
}

void CodeEditor::paintEvent(QPaintEvent *event)
{
    QPlainTextEdit::paintEvent(event);
    m_multiCursor.paint(event);

    QPainter painter(viewport());
    painter.setPen(QColor(128, 128, 128));
    QTextBlock block = firstVisibleBlock();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    while (block.isValid()) {
        const int bottom = top + qRound(blockBoundingRect(block).height());
        if (block.isVisible() && m_folding.isFolded(block.blockNumber())) {
            const QRectF lineRect = blockBoundingGeometry(block).translated(contentOffset());
            painter.drawText(lineRect.adjusted(4, 0, -4, 0),
                             Qt::AlignRight | Qt::AlignVCenter,
                             QStringLiteral("…"));
        }
        block = block.next();
        top = bottom;
    }
}

void CodeEditor::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Control) {
        viewport()->setCursor(Qt::PointingHandCursor);
    }

    if (event->key() == Qt::Key_Space && event->modifiers() == Qt::ControlModifier) {
        emit hoverCanceled();
        emit completionRequested();
        event->accept();
        return;
    }

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

    const QString typed = event->text();
    if (typed == QLatin1String("(") || typed == QLatin1String(",")) {
        emit signatureHelpRequested();
        return;
    }
    if (typed == QLatin1String(">")) {
        const QTextCursor cursor = textCursor();
        if (cursor.position() >= 2) {
            const QString around = toPlainText().mid(cursor.position() - 2, 2);
            if (around == QLatin1String("->")) {
                emit completionRequested();
            }
        }
        return;
    }
    if (!typed.isEmpty()) {
        const QChar ch = typed.at(0);
        if (ch == QLatin1Char('.') || ch == QLatin1Char(':') || ch.isLetterOrNumber()
            || ch == QLatin1Char('_')) {
            emit completionRequested();
        }
    }
}

void CodeEditor::keyReleaseEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Control) {
        viewport()->setCursor(Qt::IBeamCursor);
    }
    QPlainTextEdit::keyReleaseEvent(event);
}

void CodeEditor::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() & Qt::ControlModifier) {
        const int dy = event->angleDelta().y();
        if (dy != 0) {
            emit fontZoomRequested(dy > 0 ? 1 : -1);
            event->accept();
            return;
        }
    }
    QPlainTextEdit::wheelEvent(event);
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

void CodeEditor::scheduleFoldRefresh()
{
    if (m_foldTimer) {
        m_foldTimer->start();
    }
}

void CodeEditor::refreshFolds()
{
    m_folding.refresh();
    if (m_lineNumberArea) {
        m_lineNumberArea->update();
    }
}

void CodeEditor::toggleFold()
{
    m_folding.toggleAtCursor();
    if (m_lineNumberArea) {
        m_lineNumberArea->update();
    }
}

void CodeEditor::fold()
{
    m_folding.fold();
    if (m_lineNumberArea) {
        m_lineNumberArea->update();
    }
}

void CodeEditor::unfold()
{
    m_folding.unfold();
    if (m_lineNumberArea) {
        m_lineNumberArea->update();
    }
}

void CodeEditor::foldAll()
{
    m_folding.foldAll();
    if (m_lineNumberArea) {
        m_lineNumberArea->update();
    }
}

void CodeEditor::unfoldAll()
{
    m_folding.unfoldAll();
    if (m_lineNumberArea) {
        m_lineNumberArea->update();
    }
}

void CodeEditor::unfoldLine(int line)
{
    m_folding.unfoldLine(line);
    if (m_lineNumberArea) {
        m_lineNumberArea->update();
    }
}
