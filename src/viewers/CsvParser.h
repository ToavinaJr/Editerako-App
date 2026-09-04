#ifndef EDITERAKO_CSVPARSER_H
#define EDITERAKO_CSVPARSER_H

#include <QChar>
#include <QString>
#include <QStringList>
#include <QVector>

[[nodiscard]] QVector<QStringList> parseCsv(const QString &text,
                                            QChar delimiter = QLatin1Char(','));

#endif
