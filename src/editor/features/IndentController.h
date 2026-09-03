#ifndef EDITERAKO_INDENTCONTROLLER_H
#define EDITERAKO_INDENTCONTROLLER_H

class QKeyEvent;
class QPlainTextEdit;

class IndentController
{
public:
    explicit IndentController(QPlainTextEdit *editor);

    bool handleKeyPress(QKeyEvent *event);
    void indent();
    void outdent();
    void convertToSpaces();
    void convertToTabs();

private:
    QPlainTextEdit *m_editor = nullptr;
};

#endif
