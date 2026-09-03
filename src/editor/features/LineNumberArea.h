#ifndef EDITERAKO_LINENUMBERAREA_H
#define EDITERAKO_LINENUMBERAREA_H

#include <QWidget>

class CodeEditor;

class LineNumberArea : public QWidget
{
public:
    explicit LineNumberArea(CodeEditor *editor);

    [[nodiscard]] QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    CodeEditor *m_editor = nullptr;
};

#endif
