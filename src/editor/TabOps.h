#ifndef EDITERAKO_TABOPS_H
#define EDITERAKO_TABOPS_H

#include <QList>

struct TabCloseFlags {
    bool pinned = false;
    bool modified = false;
};

[[nodiscard]] QList<int> tabIndicesCloseToRight(int from, const QList<TabCloseFlags> &tabs);
[[nodiscard]] QList<int> tabIndicesCloseSaved(const QList<TabCloseFlags> &tabs);
[[nodiscard]] QList<int> tabIndicesCloseOthers(int keep, const QList<TabCloseFlags> &tabs);

#endif
