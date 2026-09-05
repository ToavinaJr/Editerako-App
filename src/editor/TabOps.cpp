#include "editor/TabOps.h"

QList<int> tabIndicesCloseToRight(int from, const QList<TabCloseFlags> &tabs)
{
    QList<int> out;
    if (from < 0) {
        return out;
    }
    for (int i = from + 1; i < tabs.size(); ++i) {
        if (!tabs.at(i).pinned) {
            out.append(i);
        }
    }
    return out;
}

QList<int> tabIndicesCloseSaved(const QList<TabCloseFlags> &tabs)
{
    QList<int> out;
    for (int i = 0; i < tabs.size(); ++i) {
        if (!tabs.at(i).pinned && !tabs.at(i).modified) {
            out.append(i);
        }
    }
    return out;
}

QList<int> tabIndicesCloseOthers(int keep, const QList<TabCloseFlags> &tabs)
{
    QList<int> out;
    for (int i = 0; i < tabs.size(); ++i) {
        if (i != keep && !tabs.at(i).pinned) {
            out.append(i);
        }
    }
    return out;
}
