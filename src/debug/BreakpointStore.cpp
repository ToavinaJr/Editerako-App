#include "debug/BreakpointStore.h"

#include "debug/DapTypes.h"

#include <algorithm>

QString BreakpointStore::normalizePath(const QString &path)
{
    return dapNormalizePath(path);
}

bool BreakpointStore::toggle(const QString &path, int line0)
{
    if (line0 < 0) {
        return false;
    }
    const QString key = normalizePath(path);
    if (key.isEmpty()) {
        return false;
    }

    QSet<int> &lines = m_lines[key];
    const bool added = !lines.contains(line0);
    if (added) {
        lines.insert(line0);
    } else {
        lines.remove(line0);
        if (lines.isEmpty()) {
            m_lines.remove(key);
        }
    }
    return added;
}

bool BreakpointStore::has(const QString &path, int line0) const
{
    return linesFor(path).contains(line0);
}

QSet<int> BreakpointStore::linesFor(const QString &path) const
{
    return m_lines.value(normalizePath(path));
}

QList<int> BreakpointStore::sortedLinesFor(const QString &path) const
{
    QList<int> lines = QList<int>(linesFor(path).begin(), linesFor(path).end());
    std::sort(lines.begin(), lines.end());
    return lines;
}
