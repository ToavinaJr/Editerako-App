#ifndef EDITERAKO_BREAKPOINTSTORE_H
#define EDITERAKO_BREAKPOINTSTORE_H

#include <QHash>
#include <QList>
#include <QSet>
#include <QString>

class BreakpointStore
{
public:
    [[nodiscard]] static QString normalizePath(const QString &path);

    bool toggle(const QString &path, int line0);
    [[nodiscard]] bool has(const QString &path, int line0) const;
    [[nodiscard]] QSet<int> linesFor(const QString &path) const;
    [[nodiscard]] QList<int> sortedLinesFor(const QString &path) const;
    [[nodiscard]] QHash<QString, QSet<int>> all() const { return m_lines; }
    [[nodiscard]] bool isEmpty() const { return m_lines.isEmpty(); }

private:
    QHash<QString, QSet<int>> m_lines;
};

#endif
