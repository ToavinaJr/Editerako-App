#ifndef GOTOLINEDIALOG_H
#define GOTOLINEDIALOG_H

#include <QDialog>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QPushButton;
class QLabel;
QT_END_NAMESPACE

class CodeEditor;

class GoToLineDialog : public QDialog
{
    Q_OBJECT
public:
    GoToLineDialog(CodeEditor *editor, QWidget *parent = nullptr);

private slots:
    void goToLine();
    void validateLineNumber();

private:
    CodeEditor *editor;
    QLineEdit *lineNumberEdit;
    QLabel *maxLineLabel;
    QPushButton *goButton;
    QPushButton *cancelButton;
};

#endif // GOTOLINEDIALOG_H