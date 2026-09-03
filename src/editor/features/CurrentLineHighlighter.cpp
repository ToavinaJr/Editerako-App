#include "editor/features/CurrentLineHighlighter.h"

#include <QColor>
#include <QPlainTextEdit>
#include <QTextCursor>
#include <QTextFormat>

void CurrentLineHighlighter::apply(QPlainTextEdit *editor,
                                   const QList<QTextEdit::ExtraSelection> &additional)
{
    if (!editor) {
        return;
    }

    QList<QTextEdit::ExtraSelection> extraSelections = additional;
    if (!editor->isReadOnly()) {
        QTextEdit::ExtraSelection selection;
        QColor lineColor = QColor(Qt::yellow).lighter(160);
        lineColor.setAlpha(30);
        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = editor->textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }
    editor->setExtraSelections(extraSelections);
}
