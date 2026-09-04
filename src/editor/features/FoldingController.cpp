#include "editor/features/FoldingController.h"

#include "editor/CodeEditor.h"
#include "syntax/FoldRanges.h"
#include "syntax/SyntaxHighlighter.h"
#include "syntax/TreeSitterDocument.h"

#include <QList>
#include <QSet>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QtGlobal>
#include <QVariant>

namespace {

constexpr auto kFoldedProperty = "editerakoFoldedStarts";

TreeSitterDocument *treeFor(CodeEditor *editor)
{
    if (!editor || !editor->document()) {
        return nullptr;
    }
    const auto highlighters = editor->document()->findChildren<SyntaxHighlighter *>(
        QString(), Qt::FindDirectChildrenOnly);
    if (highlighters.isEmpty()) {
        return nullptr;
    }
    return highlighters.front()->treeDocument();
}

} // namespace

FoldingController::FoldingController(CodeEditor *editor)
    : m_editor(editor)
{
}

void FoldingController::refresh()
{
    m_ranges.clear();
    TreeSitterDocument *tree = treeFor(m_editor);
    if (tree && tree->isReady()) {
        m_ranges = foldRangesFromTree(tree->rootNode());
    }

    QList<int> kept;
    for (int start : foldedStarts()) {
        if (rangeStartingAt(start)) {
            kept.append(start);
        }
    }
    setFoldedStarts(kept);
    apply();
}

void FoldingController::apply()
{
    if (!m_editor || !m_editor->document()) {
        return;
    }

    QSet<int> hidden;
    for (int start : foldedStarts()) {
        const FoldRange *range = rangeStartingAt(start);
        if (!range) {
            continue;
        }
        for (int line = range->startLine + 1; line <= range->endLine; ++line) {
            hidden.insert(line);
        }
    }

    QTextDocument *document = m_editor->document();
    for (QTextBlock block = document->firstBlock(); block.isValid(); block = block.next()) {
        const bool visible = !hidden.contains(block.blockNumber());
        if (block.isVisible() != visible) {
            block.setVisible(visible);
        }
    }
    document->markContentsDirty(0, qMax(1, document->characterCount()));
    m_editor->viewport()->update();

    QTextCursor cursor = m_editor->textCursor();
    if (!cursor.block().isVisible()) {
        const FoldRange *cover = innermostCovering(cursor.blockNumber(), true);
        if (cover) {
            const QTextBlock start = document->findBlockByNumber(cover->startLine);
            if (start.isValid()) {
                cursor.setPosition(start.position());
                m_editor->setTextCursor(cursor);
            }
        }
    }
}

void FoldingController::toggleAt(int line)
{
    if (isFolded(line)) {
        QList<int> starts = foldedStarts();
        starts.removeAll(line);
        setFoldedStarts(starts);
        apply();
        return;
    }
    if (isFoldable(line)) {
        QList<int> starts = foldedStarts();
        if (!starts.contains(line)) {
            starts.append(line);
            setFoldedStarts(starts);
            apply();
        }
    }
}

void FoldingController::toggleAtCursor()
{
    if (!m_editor) {
        return;
    }
    toggleAt(m_editor->textCursor().blockNumber());
}

void FoldingController::fold()
{
    if (!m_editor) {
        return;
    }
    const int line = m_editor->textCursor().blockNumber();
    if (isFoldable(line) && !isFolded(line)) {
        toggleAt(line);
        return;
    }
    const FoldRange *range = innermostCovering(line, false);
    if (range && !isFolded(range->startLine)) {
        toggleAt(range->startLine);
    }
}

void FoldingController::unfold()
{
    if (!m_editor) {
        return;
    }
    const int line = m_editor->textCursor().blockNumber();
    if (isFolded(line)) {
        toggleAt(line);
        return;
    }
    const FoldRange *range = innermostCovering(line, true);
    if (range) {
        toggleAt(range->startLine);
    }
}

void FoldingController::foldAll()
{
    QList<int> starts;
    starts.reserve(m_ranges.size());
    for (const FoldRange &range : m_ranges) {
        starts.append(range.startLine);
    }
    setFoldedStarts(starts);
    apply();
}

void FoldingController::unfoldAll()
{
    setFoldedStarts({});
    apply();
}

void FoldingController::unfoldLine(int line)
{
    QList<int> starts = foldedStarts();
    QList<int> kept;
    kept.reserve(starts.size());
    bool changed = false;
    for (int start : starts) {
        const FoldRange *range = rangeStartingAt(start);
        if (range && range->covers(line)) {
            changed = true;
            continue;
        }
        kept.append(start);
    }
    if (changed) {
        setFoldedStarts(kept);
        apply();
    }
}

bool FoldingController::isFoldable(int line) const
{
    return rangeStartingAt(line) != nullptr;
}

bool FoldingController::isFolded(int line) const
{
    return foldedStarts().contains(line);
}

const FoldRange *FoldingController::rangeStartingAt(int line) const
{
    for (const FoldRange &range : m_ranges) {
        if (range.startLine == line) {
            return &range;
        }
    }
    return nullptr;
}

const FoldRange *FoldingController::innermostCovering(int line, bool foldedOnly) const
{
    const FoldRange *best = nullptr;
    for (const FoldRange &range : m_ranges) {
        if (!range.covers(line)) {
            continue;
        }
        if (foldedOnly && !isFolded(range.startLine)) {
            continue;
        }
        if (!best || (range.endLine - range.startLine) < (best->endLine - best->startLine)) {
            best = &range;
        }
    }
    return best;
}

void FoldingController::setFoldedStarts(const QList<int> &starts)
{
    if (!m_editor || !m_editor->document()) {
        return;
    }
    m_editor->document()->setProperty(kFoldedProperty, QVariant::fromValue(starts));
}

QList<int> FoldingController::foldedStarts() const
{
    if (!m_editor || !m_editor->document()) {
        return {};
    }
    return m_editor->document()->property(kFoldedProperty).value<QList<int>>();
}
