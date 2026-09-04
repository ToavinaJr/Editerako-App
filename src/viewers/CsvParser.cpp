#include "viewers/CsvParser.h"

QVector<QStringList> parseCsv(const QString &text, QChar delimiter)
{
    QVector<QStringList> rows;
    QStringList row;
    QString field;
    bool inQuotes = false;

    const int n = text.size();
    for (int i = 0; i < n; ++i) {
        const QChar c = text.at(i);
        if (inQuotes) {
            if (c == QLatin1Char('"')) {
                if (i + 1 < n && text.at(i + 1) == QLatin1Char('"')) {
                    field.append(QLatin1Char('"'));
                    ++i;
                } else {
                    inQuotes = false;
                }
            } else {
                field.append(c);
            }
            continue;
        }

        if (c == QLatin1Char('"')) {
            inQuotes = true;
            continue;
        }
        if (c == delimiter) {
            row.append(field);
            field.clear();
            continue;
        }
        if (c == QLatin1Char('\r')) {
            if (i + 1 < n && text.at(i + 1) == QLatin1Char('\n')) {
                continue;
            }
            row.append(field);
            field.clear();
            rows.append(row);
            row.clear();
            continue;
        }
        if (c == QLatin1Char('\n')) {
            row.append(field);
            field.clear();
            rows.append(row);
            row.clear();
            continue;
        }
        field.append(c);
    }

    if (inQuotes || !field.isEmpty() || !row.isEmpty()) {
        row.append(field);
        rows.append(row);
    }
    return rows;
}
