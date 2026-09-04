#include "syntax/FoldRanges.h"

#include <QHash>

#include <algorithm>

namespace {

void collectNamed(TSNode node, bool isRoot, QHash<int, int> *bestEnd)
{
    if (!bestEnd || ts_node_is_null(node)) {
        return;
    }

    const TSPoint start = ts_node_start_point(node);
    const TSPoint end = ts_node_end_point(node);
    int endLine = static_cast<int>(end.row);
    if (end.column == 0 && endLine > static_cast<int>(start.row)) {
        --endLine;
    }

    if (!isRoot && ts_node_is_named(node) && !ts_node_is_error(node) && !ts_node_is_missing(node)
        && endLine > static_cast<int>(start.row)) {
        const int startLine = static_cast<int>(start.row);
        const auto it = bestEnd->constFind(startLine);
        if (it == bestEnd->cend() || endLine > it.value()) {
            bestEnd->insert(startLine, endLine);
        }
    }

    const uint32_t n = ts_node_child_count(node);
    for (uint32_t i = 0; i < n; ++i) {
        collectNamed(ts_node_child(node, i), false, bestEnd);
    }
}

} // namespace

QVector<FoldRange> foldRangesFromTree(TSNode root)
{
    QHash<int, int> bestEnd;
    collectNamed(root, true, &bestEnd);

    QVector<FoldRange> ranges;
    ranges.reserve(bestEnd.size());
    for (auto it = bestEnd.cbegin(); it != bestEnd.cend(); ++it) {
        FoldRange range;
        range.startLine = it.key();
        range.endLine = it.value();
        if (range.isValid()) {
            ranges.append(range);
        }
    }
    std::sort(ranges.begin(), ranges.end(),
              [](const FoldRange &a, const FoldRange &b) { return a.startLine < b.startLine; });
    return ranges;
}
