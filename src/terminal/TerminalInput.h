#ifndef EDITERAKO_TERMINALINPUT_H
#define EDITERAKO_TERMINALINPUT_H

#include <QKeyEvent>
#include <QListWidget>
#include <QPoint>
#include <QString>
#include <QStringList>
#include <QTextEdit>

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
    int promptPosition = 0;
    QString currentPrompt;
    AutoCompletePopup *autoCompletePopup = nullptr;

    void ensureCursorInEditableArea();
};

#endif
