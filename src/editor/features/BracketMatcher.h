#ifndef EDITERAKO_BRACKETMATCHER_H
#define EDITERAKO_BRACKETMATCHER_H

#include <QChar>
#include <QList>
#include <QString>

struct BracketMatch {
    int open = -1;
    int close = -1;
    [[nodiscard]] bool isValid() const { return open >= 0 && close >= 0; }
};

[[nodiscard]] QChar closingBracket(QChar open);
[[nodiscard]] BracketMatch findBracketMatch(const QString &text, int position);

#endif
