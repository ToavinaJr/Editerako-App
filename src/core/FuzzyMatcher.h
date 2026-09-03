#ifndef EDITERAKO_FUZZYMATCHER_H
#define EDITERAKO_FUZZYMATCHER_H

#include <QList>
#include <QString>
#include <QStringList>
#include <QStringView>

struct FuzzyMatch {
    int index = -1;
    int score = 0;
};

struct FileLineQuery {
    QString pattern;
    int line = 0;
};

[[nodiscard]] int fuzzyScore(QStringView candidate, QStringView query);
[[nodiscard]] QList<FuzzyMatch> fuzzyRank(const QStringList &candidates,
                                          const QString &query,
                                          int limit = 80);
[[nodiscard]] FileLineQuery parseFileLineQuery(const QString &text);

#endif
