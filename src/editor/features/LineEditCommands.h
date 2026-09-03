#ifndef EDITERAKO_LINEEDITCOMMANDS_H
#define EDITERAKO_LINEEDITCOMMANDS_H

class QPlainTextEdit;

class LineEditCommands
{
public:
    explicit LineEditCommands(QPlainTextEdit *editor);

    void duplicateLine();
    void deleteLine();
    void selectLine();
    void joinLines();
    void sortLines();
    void trimTrailingWhitespace();

private:
    QPlainTextEdit *m_editor = nullptr;
};

#endif
