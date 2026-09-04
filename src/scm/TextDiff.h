#ifndef EDITERAKO_TEXTDIFF_H
#define EDITERAKO_TEXTDIFF_H

#include <QString>
#include <QStringList>
#include <QVector>

struct DiffLine {
    enum class Kind {
        Equal,
        Insert,
        Delete,
    };

    Kind kind = Kind::Equal;
    QString text;
    int leftNumber = 0;
    int rightNumber = 0;
};

namespace TextDiff {
[[nodiscard]] QStringList splitLines(const QString &text);
[[nodiscard]] QVector<DiffLine> diffLines(const QString &left, const QString &right);
[[nodiscard]] QString unified(const QString &left, const QString &right,
                              const QString &leftName = QStringLiteral("a"),
                              const QString &rightName = QStringLiteral("b"));
}

#endif
