#include "scm/TextDiff.h"

#include <algorithm>
#include <vector>

QStringList TextDiff::splitLines(const QString &text)
{
    if (text.isEmpty()) {
        return {};
    }
    QString normalized = text;
    normalized.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    normalized.replace(QLatin1Char('\r'), QLatin1Char('\n'));
    QStringList lines = normalized.split(QLatin1Char('\n'));
    if (!lines.isEmpty() && lines.last().isEmpty()) {
        lines.removeLast();
    }
    return lines;
}

QVector<DiffLine> TextDiff::diffLines(const QString &left, const QString &right)
{
    const QStringList a = splitLines(left);
    const QStringList b = splitLines(right);
    const int n = static_cast<int>(a.size());
    const int m = static_cast<int>(b.size());

    QVector<DiffLine> result;
    const qint64 cells = static_cast<qint64>(n + 1) * static_cast<qint64>(m + 1);
    if (cells > 4000000) {
        int leftNo = 1;
        int rightNo = 1;
        for (const QString &line : a) {
            result.append({DiffLine::Kind::Delete, line, leftNo++, 0});
        }
        for (const QString &line : b) {
            result.append({DiffLine::Kind::Insert, line, 0, rightNo++});
        }
        return result;
    }

    std::vector<int> prev(static_cast<size_t>(m + 1), 0);
    std::vector<int> curr(static_cast<size_t>(m + 1), 0);
    std::vector<std::vector<unsigned char>> dir(
        static_cast<size_t>(n + 1), std::vector<unsigned char>(static_cast<size_t>(m + 1), 0));

    for (int i = 1; i <= n; ++i) {
        for (int j = 1; j <= m; ++j) {
            if (a.at(i - 1) == b.at(j - 1)) {
                curr[static_cast<size_t>(j)] = prev[static_cast<size_t>(j - 1)] + 1;
                dir[static_cast<size_t>(i)][static_cast<size_t>(j)] = 1;
            } else if (prev[static_cast<size_t>(j)] >= curr[static_cast<size_t>(j - 1)]) {
                curr[static_cast<size_t>(j)] = prev[static_cast<size_t>(j)];
                dir[static_cast<size_t>(i)][static_cast<size_t>(j)] = 2;
            } else {
                curr[static_cast<size_t>(j)] = curr[static_cast<size_t>(j - 1)];
                dir[static_cast<size_t>(i)][static_cast<size_t>(j)] = 3;
            }
        }
        prev.swap(curr);
        std::fill(curr.begin(), curr.end(), 0);
    }

    int i = n;
    int j = m;
    while (i > 0 || j > 0) {
        if (i > 0 && j > 0 && dir[static_cast<size_t>(i)][static_cast<size_t>(j)] == 1) {
            result.prepend({DiffLine::Kind::Equal, a.at(i - 1), i, j});
            --i;
            --j;
        } else if (j > 0 && (i == 0 || dir[static_cast<size_t>(i)][static_cast<size_t>(j)] == 3)) {
            result.prepend({DiffLine::Kind::Insert, b.at(j - 1), 0, j});
            --j;
        } else if (i > 0) {
            result.prepend({DiffLine::Kind::Delete, a.at(i - 1), i, 0});
            --i;
        }
    }
    return result;
}

QString TextDiff::unified(const QString &left, const QString &right, const QString &leftName,
                          const QString &rightName)
{
    const QVector<DiffLine> lines = diffLines(left, right);
    bool any = false;
    for (const DiffLine &line : lines) {
        if (line.kind != DiffLine::Kind::Equal) {
            any = true;
            break;
        }
    }
    if (!any) {
        return {};
    }

    QString out;
    out += QStringLiteral("--- %1\n+++ %2\n@@\n").arg(leftName, rightName);
    for (const DiffLine &line : lines) {
        switch (line.kind) {
        case DiffLine::Kind::Equal:
            out += QLatin1Char(' ');
            break;
        case DiffLine::Kind::Delete:
            out += QLatin1Char('-');
            break;
        case DiffLine::Kind::Insert:
            out += QLatin1Char('+');
            break;
        }
        out += line.text;
        out += QLatin1Char('\n');
    }
    if (out.endsWith(QLatin1Char('\n'))) {
        out.chop(1);
    }
    return out;
}
