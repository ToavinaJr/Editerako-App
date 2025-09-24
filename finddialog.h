#ifndef FINDREPLACEDIALOG_H
#define FINDREPLACEDIALOG_H

#include <QDialog>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QCheckBox;
class QPushButton;
QT_END_NAMESPACE

class CodeEditor;

class FindReplaceDialog : public QDialog
{
    Q_OBJECT
public:
    FindReplaceDialog(CodeEditor *editor, QWidget *parent = nullptr);

private slots:
    void findNext();
    void replace();
    void replaceAll();

private:
    CodeEditor *editor;
    QLineEdit *searchLineEdit;
    QLineEdit *replaceLineEdit;
    QCheckBox *caseSensitiveCheckBox;
    QCheckBox *regexCheckBox;
    QPushButton *findNextButton;
    QPushButton *replaceButton;
    QPushButton *replaceAllButton;
    QPushButton *cancelButton;
};

#endif // FINDREPLACEDIALOG_H
