#ifndef EDITERAKO_TERMINALPANEL_H
#define EDITERAKO_TERMINALPANEL_H

#include <QList>
#include <QString>
#include <QWidget>

class QPushButton;
class QTabWidget;
class Terminal;

class TerminalPanel : public QWidget
{
    Q_OBJECT

public:
    explicit TerminalPanel(QWidget *parent = nullptr);

    void setWorkingDirectory(const QString &path);
    void setCurrentWorkingDirectory(const QString &cwd);
    void toggle(const QString &focusCwd);
    void addTerminal(const QString &cwd);
    void shutdownAll();

signals:
    void addRequested();
    void currentTabChanged();
    void editorFocusRequested();

private:
    void closeTab(int index);
    void attachCloseButton(Terminal *terminal);
    void updateTabTitles();
    void connectClosed(Terminal *terminal);

    QTabWidget *m_tabs = nullptr;
    QList<Terminal *> m_terminals;
    QPushButton *m_addButton = nullptr;
    bool m_userVisible = false;
};

#endif
