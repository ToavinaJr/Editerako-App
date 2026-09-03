#ifndef EDITERAKO_LINEBLOCKOPS_H
#define EDITERAKO_LINEBLOCKOPS_H

#include <QString>
#include <QStringList>

class QPlainTextEdit;

struct LineRange {
    int firstBlock = 0;
    int lastBlock = 0;
};

[[nodiscard]] LineRange selectedLineRange(QPlainTextEdit *editor);
[[nodiscard]] QStringList selectedLines(QPlainTextEdit *editor, const LineRange &range);
void replaceSelectedLines(QPlainTextEdit *editor, const LineRange &range, const QStringList &lines);
void replaceDocumentText(QPlainTextEdit *editor, const QString &text);

#endif
