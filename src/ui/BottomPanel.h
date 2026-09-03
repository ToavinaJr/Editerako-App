#ifndef EDITERAKO_BOTTOMPANEL_H
#define EDITERAKO_BOTTOMPANEL_H

#include <QWidget>

class ProblemsPanel;
class QTabWidget;
class TerminalPanel;

class BottomPanel : public QWidget
{
    Q_OBJECT

public:
    explicit BottomPanel(QWidget *parent = nullptr);

    [[nodiscard]] ProblemsPanel *problemsPanel() const { return m_problems; }
    [[nodiscard]] TerminalPanel *terminalPanel() const { return m_terminal; }

    void toggleTerminal(const QString &focusCwd);
    void showProblems();
    void toggleProblems();
    void showTerminal();
    void updateProblemsTitle();

signals:
    void editorFocusRequested();

private:
    void showAndSelect(int index);

    QTabWidget *m_tabs = nullptr;
    ProblemsPanel *m_problems = nullptr;
    TerminalPanel *m_terminal = nullptr;
    int m_problemsIndex = 0;
    int m_terminalIndex = 1;
    bool m_userVisible = true;
};

#endif
