#include "terminal/TerminalPanel.h"

#include "terminal/Terminal.h"

#include <QHBoxLayout>
#include <QPushButton>
#include <QTabBar>
#include <QTabWidget>
#include <QVBoxLayout>

TerminalPanel::TerminalPanel(QWidget *parent)
    : QWidget(parent)
{
    m_tabs = new QTabWidget(this);
    m_tabs->setObjectName(QStringLiteral("terminalTabs"));
    m_tabs->setMovable(true);

    Terminal *firstTerminal = new Terminal(this);
    m_terminals.append(firstTerminal);
    connectClosed(firstTerminal);

    m_tabs->addTab(firstTerminal, QStringLiteral("⚡ Terminal 1"));
    attachCloseButton(firstTerminal);

    m_addButton = new QPushButton(QStringLiteral("+"), this);
    m_addButton->setObjectName(QStringLiteral("addTerminalButton"));
    m_addButton->setFixedSize(28, 28);
    m_addButton->setToolTip(tr("Add new terminal"));
    m_addButton->setCursor(Qt::PointingHandCursor);

    auto *cornerWidget = new QWidget(this);
    auto *cornerLayout = new QHBoxLayout(cornerWidget);
    cornerLayout->setContentsMargins(0, 0, 15, 0);
    cornerLayout->setSpacing(0);
    cornerLayout->addWidget(m_addButton);
    m_tabs->setCornerWidget(cornerWidget, Qt::TopRightCorner);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_tabs);

    setMinimumHeight(250);
    setMaximumHeight(400);

    connect(m_addButton, &QPushButton::clicked, this, &TerminalPanel::addRequested);
    connect(m_tabs, &QTabWidget::currentChanged, this, &TerminalPanel::currentTabChanged);

    firstTerminal->setWorkingDirectory(QString());
    firstTerminal->setVisible(true);
    m_userVisible = true;
    setVisible(false);
}

void TerminalPanel::setWorkingDirectory(const QString &path)
{
    for (Terminal *terminal : m_terminals) {
        if (terminal) {
            terminal->setWorkingDirectory(path);
        }
    }
}

void TerminalPanel::setCurrentWorkingDirectory(const QString &cwd)
{
    const int index = m_tabs ? m_tabs->currentIndex() : -1;
    if (index >= 0 && index < m_terminals.size()) {
        m_terminals.at(index)->setWorkingDirectory(cwd);
    }
}

void TerminalPanel::toggle(const QString &focusCwd)
{
    if (!m_userVisible) {
        if (m_terminals.isEmpty()) {
            addTerminal(focusCwd);
        }
        m_userVisible = true;
        setVisible(true);

        const int index = m_tabs ? m_tabs->currentIndex() : -1;
        if (index >= 0 && index < m_terminals.size()) {
            Terminal *currentTerminal = m_terminals.at(index);
            currentTerminal->setWorkingDirectory(focusCwd);
            currentTerminal->focusTerminal();
        }
    } else {
        m_userVisible = false;
        setVisible(false);
        emit editorFocusRequested();
    }
}

void TerminalPanel::addTerminal(const QString &cwd)
{
    Terminal *newTerminal = new Terminal(this);
    m_terminals.append(newTerminal);
    connectClosed(newTerminal);

    newTerminal->setWorkingDirectory(cwd);

    const int tabIndex = m_tabs->addTab(newTerminal, QStringLiteral("⚡ Terminal %1").arg(m_terminals.size()));
    attachCloseButton(newTerminal);

    setVisible(true);
    m_userVisible = true;

    m_tabs->setCurrentIndex(tabIndex);
    newTerminal->focusTerminal();
}

void TerminalPanel::shutdownAll()
{
    for (Terminal *terminal : m_terminals) {
        if (terminal) {
            terminal->shutdown();
        }
    }
}

void TerminalPanel::closeTab(int index)
{
    if (index < 0 || index >= m_terminals.size()) {
        return;
    }

    Terminal *terminalToClose = m_terminals.takeAt(index);
    m_tabs->removeTab(index);

    terminalToClose->shutdown();
    terminalToClose->deleteLater();

    updateTabTitles();

    if (m_terminals.isEmpty()) {
        setVisible(false);
        m_userVisible = false;
        emit editorFocusRequested();
    }
}

void TerminalPanel::attachCloseButton(Terminal *terminal)
{
    if (!m_tabs || !terminal) {
        return;
    }

    const int index = m_tabs->indexOf(terminal);
    if (index < 0) {
        return;
    }

    QWidget *oldBtn = m_tabs->tabBar()->tabButton(index, QTabBar::RightSide);
    if (oldBtn) {
        m_tabs->tabBar()->setTabButton(index, QTabBar::RightSide, nullptr);
        oldBtn->deleteLater();
    }

    auto *closeBtn = new QPushButton(QStringLiteral("×"), m_tabs->tabBar());
    closeBtn->setObjectName(QStringLiteral("terminalCloseButton"));
    closeBtn->setFixedSize(16, 16);
    closeBtn->setCursor(Qt::PointingHandCursor);
    connect(closeBtn, &QPushButton::clicked, this, [this, terminal]() {
        const int idx = m_terminals.indexOf(terminal);
        if (idx >= 0) {
            closeTab(idx);
        }
    });
    m_tabs->tabBar()->setTabButton(index, QTabBar::RightSide, closeBtn);
}

void TerminalPanel::updateTabTitles()
{
    for (int i = 0; i < m_terminals.size(); ++i) {
        m_tabs->setTabText(i, QStringLiteral("⚡ Terminal %1").arg(i + 1));
    }
}

void TerminalPanel::connectClosed(Terminal *terminal)
{
    connect(terminal, &Terminal::terminalClosed, this, [this, terminal]() {
        const int index = m_terminals.indexOf(terminal);
        if (index >= 0) {
            closeTab(index);
        }
    });
}
