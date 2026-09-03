#include "core/FuzzyMatcher.h"

#include <QChar>
#include <QtGlobal>
#include <algorithm>

namespace {

QString normalized(QStringView text)
{
    QString out = text.toString();
    out.replace(QLatin1Char('\\'), QLatin1Char('/'));
    return out.toCaseFolded();
}

} // namespace

int fuzzyScore(QStringView candidate, QStringView query)
{
    if (query.isEmpty()) {
        return 0;
    }

    const QString hay = normalized(candidate);
    const QString needle = normalized(query);
    if (needle.isEmpty()) {
        return 0;
    }

    int score = 0;
    int qi = 0;
    int last = -2;
    for (int i = 0; i < hay.size() && qi < needle.size(); ++i) {
        if (hay.at(i) != needle.at(qi)) {
            continue;
        }
        score += 8;
        if (i == last + 1) {
            score += 16;
        }
        if (i == 0) {
            score += 24;
        } else {
            const QChar prev = hay.at(i - 1);
            if (prev == QLatin1Char('/') || prev == QLatin1Char('.') || prev == QLatin1Char('_')
                || prev == QLatin1Char('-')) {
                score += 20;
            }
        }
        last = i;
        ++qi;
    }

    if (qi != needle.size()) {
        return -1;
    }

    const int slash = hay.lastIndexOf(QLatin1Char('/'));
    const QString fileName = slash >= 0 ? hay.mid(slash + 1) : hay;
    if (fileName.startsWith(needle)) {
        score += 40;
    } else if (fileName.contains(needle)) {
        score += 12;
    }
    score -= qMin(hay.size(), 80);
    return score;
}

QList<FuzzyMatch> fuzzyRank(const QStringList &candidates, const QString &query, int limit)
{
    QList<FuzzyMatch> matches;
    matches.reserve(candidates.size());
    for (int i = 0; i < candidates.size(); ++i) {
        const int score = fuzzyScore(candidates.at(i), query);
        if (score < 0) {
            continue;
        }
        FuzzyMatch match;
        match.index = i;
        match.score = score;
        matches.append(match);
    }

    std::sort(matches.begin(), matches.end(), [](const FuzzyMatch &a, const FuzzyMatch &b) {
        if (a.score != b.score) {
            return a.score > b.score;
        }
        return a.index < b.index;
    });

    if (limit > 0 && matches.size() > limit) {
        matches.resize(limit);
    }
    return matches;
}

FileLineQuery parseFileLineQuery(const QString &text)
{
    FileLineQuery result;
    result.pattern = text;
    const int colon = text.lastIndexOf(QLatin1Char(':'));
    if (colon <= 0 || colon == text.size() - 1) {
        return result;
    }
    bool ok = false;
    const int line = text.mid(colon + 1).trimmed().toInt(&ok);
    if (!ok || line <= 0) {
        return result;
    }
    result.pattern = text.left(colon);
    result.line = line;
    return result;
}
