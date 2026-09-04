#ifndef EDITERAKO_BOTTOMPANEL_H
#define EDITERAKO_BOTTOMPANEL_H

#include <QWidget>

class ProblemsPanel;
class GitCliProvider;
class SourceControlPanel;
class DiffViewer;
class TaskManager;
class TasksPanel;
class OutputPanel;
class QTabWidget;
class TerminalPanel;

class BottomPanel : public QWidget
{
    Q_OBJECT

public:
    explicit BottomPanel(GitCliProvider *scm, TaskManager *tasks, QWidget *parent = nullptr);

    [[nodiscard]] ProblemsPanel *problemsPanel() const { return m_problems; }
    [[nodiscard]] TerminalPanel *terminalPanel() const { return m_terminal; }
    [[nodiscard]] SourceControlPanel *sourceControlPanel() const { return m_sourceControl; }
    [[nodiscard]] TasksPanel *tasksPanel() const { return m_tasks; }
    [[nodiscard]] OutputPanel *outputPanel() const { return m_output; }

    void toggleTerminal(const QString &focusCwd);
    void showProblems();
    void toggleProblems();
    void toggleSourceControl();
    void toggleTasks();
    void toggleOutput();
    void showTerminal();
    void showOutput();
    void showTasks();
    void showDiff(const QString &path, const QString &text);
    void updateProblemsTitle();

signals:
    void editorFocusRequested();

private:
    void showAndSelect(int index);

    QTabWidget *m_tabs = nullptr;
    ProblemsPanel *m_problems = nullptr;
    OutputPanel *m_output = nullptr;
    TerminalPanel *m_terminal = nullptr;
    SourceControlPanel *m_sourceControl = nullptr;
    DiffViewer *m_diff = nullptr;
    TasksPanel *m_tasks = nullptr;
    int m_problemsIndex = 0;
    int m_outputIndex = 1;
    int m_terminalIndex = 2;
    int m_sourceControlIndex = 3;
    int m_diffIndex = 4;
    int m_tasksIndex = 5;
    bool m_userVisible = true;
};

#endif
