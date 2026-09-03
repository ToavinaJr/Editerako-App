#include "editor/EditorStyle.h"

#include "core/AppSettings.h"
#include "editor/CodeEditor.h"

#include <QFont>
#include <QFontMetrics>
#include <QPlainTextEdit>
#include <QtGlobal>

void EditorStyle::apply(CodeEditor *editor)
{
    if (!editor) {
        return;
    }

    const AppSettings settings;
    QFont font(settings.editorFontFamily());
    font.setPointSize(qMax(8, settings.editorFontSize()));
    editor->setFont(font);

    const QFontMetrics metrics(font);
    editor->setTabStopDistance(
        settings.editorTabSize() * metrics.horizontalAdvance(QLatin1Char(' ')));
    editor->setLineWrapMode(settings.editorWordWrap()
                                ? QPlainTextEdit::WidgetWidth
                                : QPlainTextEdit::NoWrap);
    editor->setLineNumbersVisible(settings.editorLineNumbers());
}
