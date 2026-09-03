#ifndef EDITERAKO_CURRENTLINEHIGHLIGHTER_H
#define EDITERAKO_CURRENTLINEHIGHLIGHTER_H

#include <QList>
#include <QTextEdit>

class QPlainTextEdit;

namespace CurrentLineHighlighter {

void apply(QPlainTextEdit *editor,
           const QList<QTextEdit::ExtraSelection> &additional = {});

}

#endif
