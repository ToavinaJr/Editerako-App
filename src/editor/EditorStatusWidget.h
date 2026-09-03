#ifndef EDITERAKO_EDITORSTATUSWIDGET_H
#define EDITERAKO_EDITORSTATUSWIDGET_H

#include <QWidget>

class CodeEditor;
class QLabel;

class EditorStatusWidget : public QWidget
{
    Q_OBJECT

public:
    explicit EditorStatusWidget(QWidget *parent = nullptr);

    void setEditor(CodeEditor *editor);

private:
    void refresh();

    CodeEditor *m_editor = nullptr;
    QLabel *m_position = nullptr;
    QLabel *m_encoding = nullptr;
    QLabel *m_eol = nullptr;
    QLabel *m_language = nullptr;
};

#endif
