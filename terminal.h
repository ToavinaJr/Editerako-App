#ifndef TERMINAL_H
#define TERMINAL_H

#include <QWidget>
#include <QProcess>
#include <QTextEdit>
#include <QString>
#include <QKeyEvent>

namespace Ui {
class Terminal;
}

class TerminalTextEdit : public QTextEdit
{
    Q_OBJECT

public:
    explicit TerminalTextEdit(QWidget *parent = nullptr);
    void setPrompt(const QString &prompt);
    QString getCurrentCommand() const;
    void clearCurrentCommand();

signals:
    void commandEntered(const QString &command);
    void upPressed();
    void downPressed();
    void tabPressed();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    int promptPosition;
    QString currentPrompt;

    void ensureCursorInEditableArea();
    int getPromptPosition() const;
};

class Terminal : public QWidget
{
    Q_OBJECT

public:
    explicit Terminal(QWidget *parent = nullptr);
    ~Terminal();

    void setWorkingDirectory(const QString &path);
    QString getWorkingDirectory() const;
    void clearTerminal();
    void executeCommand(const QString &command);

public slots:
    void focusTerminal();

signals:
    void terminalClosed();

private slots:
    void onCommandEntered(const QString &command);
    void onProcessReadyRead();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);
    void onUpPressed();
    void onDownPressed();
    void onClearClicked();
    void onCloseClicked();

private:
    Ui::Terminal *ui;
    QProcess *process;
    QString workingDirectory;
    QString currentShell;
    QStringList commandHistory;
    int historyIndex;
    bool isProcessRunning;
    bool isDragging;
    QPoint dragStartPosition;
    void setupTerminal();
    void displayPrompt();
    void appendOutput(const QString &text, const QColor &color = QColor(204, 204, 204));
    void initializeShell();
    QString getSystemShell();
    void navigateHistory(int direction);
    void processInternalCommand(const QString &command);
};

#endif // TERMINAL_H
