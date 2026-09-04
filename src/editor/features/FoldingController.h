#ifndef EDITERAKO_FOLDINGCONTROLLER_H
#define EDITERAKO_FOLDINGCONTROLLER_H

#include "syntax/FoldRanges.h"

#include <QList>
#include <QVector>

class CodeEditor;

inline constexpr int kBreakpointGutterWidth = 14;
inline constexpr int kFoldGutterWidth = 12;

class FoldingController
{
public:
    explicit FoldingController(CodeEditor *editor);

    void refresh();
    void apply();

    void toggleAt(int line);
    void toggleAtCursor();
    void fold();
    void unfold();
    void foldAll();
    void unfoldAll();
    void unfoldLine(int line);

    [[nodiscard]] bool isFoldable(int line) const;
    [[nodiscard]] bool isFolded(int line) const;
    [[nodiscard]] const QVector<FoldRange> &ranges() const { return m_ranges; }

private:
    [[nodiscard]] const FoldRange *rangeStartingAt(int line) const;
    [[nodiscard]] const FoldRange *innermostCovering(int line, bool foldedOnly) const;
    void setFoldedStarts(const QList<int> &starts);
    [[nodiscard]] QList<int> foldedStarts() const;

    CodeEditor *m_editor = nullptr;
    QVector<FoldRange> m_ranges;
};

#endif
