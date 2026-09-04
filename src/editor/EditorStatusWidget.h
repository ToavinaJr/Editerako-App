#ifndef EDITERAKO_EDITORSTATUSWIDGET_H
#define EDITERAKO_EDITORSTATUSWIDGET_H

#include <QWidget>

class CodeEditor;
class QLabel;
class QPushButton;

class EditorStatusWidget : public QWidget
{
    Q_OBJECT

public:
    explicit EditorStatusWidget(QWidget *parent = nullptr);

    void setEditor(CodeEditor *editor);
    void setLspStatus(const QString &text);
    void setGitBranch(const QString &text);
    void setProblemCounts(int errors, int warnings);

signals:
    void problemsActivated();

private:
    void refresh();

    CodeEditor *m_editor = nullptr;
    QLabel *m_position = nullptr;
    QLabel *m_encoding = nullptr;
    QLabel *m_eol = nullptr;
    QLabel *m_language = nullptr;
    QLabel *m_lsp = nullptr;
    QLabel *m_git = nullptr;
    QPushButton *m_problems = nullptr;
};

#endif
