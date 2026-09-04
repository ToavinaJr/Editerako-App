#ifndef EDITERAKO_TERMINAL_H
#define EDITERAKO_TERMINAL_H

#include "terminal/AnsiSgr.h"
#include "terminal/CommandCompleter.h"
#include "terminal/CommandHistory.h"

#include <QColor>
#include <QString>
#include <QWidget>

class CommandDiscovery;
class QResizeEvent;
class TerminalProcess;

namespace Ui {
class Terminal;
}

class Terminal : public QWidget
{
    Q_OBJECT

public:
    explicit Terminal(QWidget *parent = nullptr);
    ~Terminal() override;

    void setWorkingDirectory(const QString &path);
    QString getWorkingDirectory() const;
    void clearTerminal();
    void executeCommand(const QString &command);
    void shutdown();

public slots:
    void focusTerminal();

signals:
    void terminalClosed();

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onCommandEntered(const QString &command);
    void onProcessOutput(const QString &text, bool isError);
    void onProcessFinished(int exitCode, bool crashed);
    void onProcessFailed(const QString &message);
    void onUpPressed();
    void onDownPressed();
    void onClearClicked();
    void onCloseClicked();
    void onTextChangedForAutoComplete();

private:
    Ui::Terminal *ui;
    TerminalProcess *m_process = nullptr;
    CommandHistory m_history;
    CommandDiscovery *m_discovery = nullptr;
    CommandCompleter m_completer;
    AnsiSgrDecoder m_ansi;
    QString workingDirectory;

    void setupTerminal();
    void updatePtySize();
    void displayPrompt();
    void appendOutput(const QString &text, const QColor &color);
    void appendError(const QString &text);
    void appendSuccess(const QString &text);
    void appendInfo(const QString &text);
    void navigateHistory(int direction);
    void updateAutoComplete();
};

#endif
