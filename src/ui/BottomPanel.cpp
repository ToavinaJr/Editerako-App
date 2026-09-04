#include "ui/BottomPanel.h"

#include "terminal/TerminalPanel.h"
#include "ui/ProblemsPanel.h"
#include "ui/SourceControlPanel.h"
#include "ui/DiffViewer.h"
#include "scm/GitCliProvider.h"

#include <QVBoxLayout>
#include <QTabWidget>

BottomPanel::BottomPanel(GitCliProvider *scm, QWidget *parent)
    : QWidget(parent)
    , m_tabs(new QTabWidget(this))
    , m_problems(new ProblemsPanel(this))
    , m_terminal(new TerminalPanel(this))
    , m_sourceControl(new SourceControlPanel(scm, this))
    , m_diff(new DiffViewer(this))
{
    setObjectName(QStringLiteral("bottomPanel"));
    m_tabs->setObjectName(QStringLiteral("bottomTabs"));

    m_problemsIndex = m_tabs->addTab(m_problems, tr("Problems"));
    m_terminalIndex = m_tabs->addTab(m_terminal, tr("Terminal"));
    m_sourceControlIndex = m_tabs->addTab(m_sourceControl, tr("Source Control"));
    m_diffIndex = m_tabs->addTab(m_diff, tr("Diff"));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_tabs);

    setMinimumHeight(220);
    setMaximumHeight(420);

    connect(m_terminal, &TerminalPanel::hideRequested, this, [this]() {
        m_userVisible = false;
        setVisible(false);
        emit editorFocusRequested();
    });
    connect(m_terminal, &TerminalPanel::showRequested, this, [this]() {
        showAndSelect(m_terminalIndex);
    });
    connect(m_problems->model(), &ProblemModel::changed, this, &BottomPanel::updateProblemsTitle);
    connect(m_sourceControl, &SourceControlPanel::diffRequested, scm, &GitCliProvider::requestDiff);
    connect(scm, &GitCliProvider::diffReady, this, [this](const QString &path, const QString &text) {
        m_diff->setDiff(path, text);
        showAndSelect(m_diffIndex);
    });

    m_userVisible = true;
    setVisible(false);
    updateProblemsTitle();
}

void BottomPanel::showAndSelect(int index)
{
    m_userVisible = true;
    setVisible(true);
    m_tabs->setCurrentIndex(index);
}

void BottomPanel::toggleTerminal(const QString &focusCwd)
{
    if (!m_userVisible) {
        m_userVisible = true;
        setVisible(true);
        m_tabs->setCurrentIndex(m_terminalIndex);
        m_terminal->focusCurrent(focusCwd);
        return;
    }
    m_userVisible = false;
    setVisible(false);
    emit editorFocusRequested();
}

void BottomPanel::showProblems()
{
    showAndSelect(m_problemsIndex);
}

void BottomPanel::toggleProblems()
{
    if (!m_userVisible || m_tabs->currentIndex() != m_problemsIndex) {
        showProblems();
        return;
    }
    m_userVisible = false;
    setVisible(false);
    emit editorFocusRequested();
}

void BottomPanel::toggleSourceControl()
{
    if (!m_userVisible || m_tabs->currentIndex() != m_sourceControlIndex) {
        showAndSelect(m_sourceControlIndex);
        return;
    }
    m_userVisible = false;
    setVisible(false);
    emit editorFocusRequested();
}

void BottomPanel::showTerminal()
{
    showAndSelect(m_terminalIndex);
}

void BottomPanel::showDiff(const QString &path, const QString &text)
{
    m_diff->setDiff(path, text);
    showAndSelect(m_diffIndex);
}

void BottomPanel::updateProblemsTitle()
{
    const int total = m_problems->model()->totalCount();
    if (total <= 0) {
        m_tabs->setTabText(m_problemsIndex, tr("Problems"));
        return;
    }
    m_tabs->setTabText(m_problemsIndex, tr("Problems (%1)").arg(total));
}
