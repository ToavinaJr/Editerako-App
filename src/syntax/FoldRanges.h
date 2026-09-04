#ifndef EDITERAKO_FOLDRANGES_H
#define EDITERAKO_FOLDRANGES_H

#include <QVector>

#include <tree_sitter/api.h>

struct FoldRange {
    int startLine = 0;
    int endLine = 0;

    [[nodiscard]] bool isValid() const { return endLine > startLine; }
    [[nodiscard]] bool hides(int line) const { return line > startLine && line <= endLine; }
    [[nodiscard]] bool covers(int line) const { return line >= startLine && line <= endLine; }
};

[[nodiscard]] QVector<FoldRange> foldRangesFromTree(TSNode root);

#endif
