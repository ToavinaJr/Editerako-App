#include "editor/features/LineNumberArea.h"

#include "editor/CodeEditor.h"

#include <QMouseEvent>

LineNumberArea::LineNumberArea(CodeEditor *editor)
    : QWidget(editor)
    , m_editor(editor)
{
    setCursor(Qt::PointingHandCursor);
}

QSize LineNumberArea::sizeHint() const
{
    return QSize(m_editor->lineNumberAreaWidth(), 0);
}

void LineNumberArea::paintEvent(QPaintEvent *event)
{
    m_editor->lineNumberAreaPaintEvent(event);
}

void LineNumberArea::mousePressEvent(QMouseEvent *event)
{
    m_editor->gutterMousePress(event);
}
