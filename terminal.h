#ifndef TERMINAL_H
#define TERMINAL_H

#include <QWidget>
#include <QProcess>
#include <QTextEdit>
#include <QString>
#include <QKeyEvent>
#include <QListWidget>
#include <QMap>
#include <QStringList>

namespace Ui {
class Terminal;
}

// Popup widget for autocomplete suggestions
class AutoCompletePopup : public QListWidget
{
    Q_OBJECT
public:
    explicit AutoCompletePopup(QWidget *parent = nullptr);
    void showSuggestions(const QStringList &suggestions, const QPoint &position);
    QString currentSuggestion() const;

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

signals:
    void suggestionSelected(const QString &suggestion);
    void cancelled();
};

class TerminalTextEdit : public QTextEdit
{
    Q_OBJECT

public:
    explicit TerminalTextEdit(QWidget *parent = nullptr);
    void setPrompt(const QString &prompt);
    QString getCurrentCommand() const;
    void clearCurrentCommand();
    void showAutoComplete(const QStringList &suggestions);
    void hideAutoComplete();
    void acceptSuggestion(const QString &suggestion);

signals:
    void commandEntered(const QString &command);
    void upPressed();
    void downPressed();
    void tabPressed();
    void textChangedForAutoComplete();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    int promptPosition;
    QString currentPrompt;
    AutoCompletePopup *autoCompletePopup;

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
    void onTextChangedForAutoComplete();
    void onSuggestionSelected(const QString &suggestion);

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
    
    // Autocomplete data structures
    QMap<QString, QStringList> commandArguments;
    QStringList commonCommands;
    
    void setupTerminal();
    void displayPrompt();
    void appendOutput(const QString &text, const QColor &color);
    void appendError(const QString &text);
    void appendSuccess(const QString &text);
    void appendInfo(const QString &text);
    void initializeShell();
    QString getSystemShell();
    void navigateHistory(int direction);
    void processInternalCommand(const QString &command);
    
    // Autocomplete helper methods
    void initializeCommandDatabase();
    QStringList getCommandSuggestions(const QString &partial);
    QStringList getArgumentSuggestions(const QString &command, const QString &partial);
    QStringList getPathSuggestions(const QString &partial);
    void updateAutoComplete();
};

#endif // TERMINAL_H
