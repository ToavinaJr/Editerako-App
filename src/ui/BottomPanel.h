#ifndef EDITERAKO_BOTTOMPANEL_H
#define EDITERAKO_BOTTOMPANEL_H

#include <QWidget>

class ProblemsPanel;
class GitCliProvider;
class SourceControlPanel;
class DiffViewer;
class QTabWidget;
class TerminalPanel;

class BottomPanel : public QWidget
{
    Q_OBJECT

public:
    explicit BottomPanel(GitCliProvider *scm, QWidget *parent = nullptr);

    [[nodiscard]] ProblemsPanel *problemsPanel() const { return m_problems; }
    [[nodiscard]] TerminalPanel *terminalPanel() const { return m_terminal; }
    [[nodiscard]] SourceControlPanel *sourceControlPanel() const { return m_sourceControl; }

    void toggleTerminal(const QString &focusCwd);
    void showProblems();
    void toggleProblems();
    void toggleSourceControl();
    void showTerminal();
    void updateProblemsTitle();

signals:
    void editorFocusRequested();

private:
    void showAndSelect(int index);

    QTabWidget *m_tabs = nullptr;
    ProblemsPanel *m_problems = nullptr;
    TerminalPanel *m_terminal = nullptr;
    SourceControlPanel *m_sourceControl = nullptr;
    DiffViewer *m_diff = nullptr;
    int m_problemsIndex = 0;
    int m_terminalIndex = 1;
    int m_sourceControlIndex = 2;
    int m_diffIndex = 3;
    bool m_userVisible = true;
};

#endif
