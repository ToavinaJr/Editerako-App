#ifndef EDITERAKO_AUTOCLOSINGPAIRS_H
#define EDITERAKO_AUTOCLOSINGPAIRS_H

#include <QChar>
#include <QString>

[[nodiscard]] QChar closingPairFor(QChar typed);
[[nodiscard]] bool shouldInsertClosingPair(const QString &text, int position, QChar typed);
[[nodiscard]] bool shouldSkipClosingPair(const QString &text, int position, QChar typed);

#endif
