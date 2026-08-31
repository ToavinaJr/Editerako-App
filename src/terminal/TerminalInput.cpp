#include "terminal/TerminalInput.h"

#include <QAbstractItemView>
#include <QMouseEvent>
#include <QTextCursor>

AutoCompletePopup::AutoCompletePopup(QWidget *parent)
    : QListWidget(parent)
{
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setFocusPolicy(Qt::NoFocus);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setObjectName(QStringLiteral("AutoCompletePopup"));

    connect(this, &QListWidget::itemClicked, this, [this](QListWidgetItem *item) {
        if (item) {
            emit suggestionSelected(item->text());
            hide();
        }
    });
}

void AutoCompletePopup::showSuggestions(const QStringList &suggestions, const QPoint &position)
{
    clear();

    if (suggestions.isEmpty()) {
        hide();
        return;
    }

    addItems(suggestions);
    setCurrentRow(0);

    const int maxWidth = 300;
    const int itemHeight = 30;
    const int totalHeight = qMin(suggestions.count() * itemHeight + 10, 250);

    setFixedSize(maxWidth, totalHeight);
    move(position);
    show();
    raise();
}

QString AutoCompletePopup::currentSuggestion() const
{
    QListWidgetItem *item = currentItem();
    return item ? item->text() : QString();
}

void AutoCompletePopup::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        emit cancelled();
        hide();
        return;
    }

    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter || event->key() == Qt::Key_Tab) {
        const QString suggestion = currentSuggestion();
        if (!suggestion.isEmpty()) {
            emit suggestionSelected(suggestion);
            hide();
        }
        return;
    }

    QListWidget::keyPressEvent(event);
}

void AutoCompletePopup::focusOutEvent(QFocusEvent *event)
{
    Q_UNUSED(event);
    hide();
}

TerminalTextEdit::TerminalTextEdit(QWidget *parent)
    : QTextEdit(parent)
    , promptPosition(0)
{
    setAcceptRichText(false);
    setUndoRedoEnabled(false);

    autoCompletePopup = new AutoCompletePopup(this);
    connect(autoCompletePopup, &AutoCompletePopup::suggestionSelected,
            this, &TerminalTextEdit::acceptSuggestion);
    connect(autoCompletePopup, &AutoCompletePopup::cancelled,
            this, &TerminalTextEdit::hideAutoComplete);
}

void TerminalTextEdit::setPrompt(const QString &prompt)
{
    currentPrompt = prompt;
    promptPosition = textCursor().position();
}

QString TerminalTextEdit::getCurrentCommand() const
{
    const QString fullText = toPlainText();
    if (fullText.length() <= promptPosition) {
        return {};
    }
    return fullText.mid(promptPosition);
}

void TerminalTextEdit::clearCurrentCommand()
{
    QTextCursor cursor = textCursor();
    cursor.setPosition(promptPosition);
    cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
    cursor.removeSelectedText();
}

void TerminalTextEdit::keyPressEvent(QKeyEvent *event)
{
    if (autoCompletePopup->isVisible()) {
        if (event->key() == Qt::Key_Up) {
            const int currentRow = autoCompletePopup->currentRow();
            if (currentRow > 0) {
                autoCompletePopup->setCurrentRow(currentRow - 1);
            }
            return;
        }

        if (event->key() == Qt::Key_Down) {
            const int currentRow = autoCompletePopup->currentRow();
            if (currentRow < autoCompletePopup->count() - 1) {
                autoCompletePopup->setCurrentRow(currentRow + 1);
            }
            return;
        }

        if (event->key() == Qt::Key_Tab || event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
            const QString suggestion = autoCompletePopup->currentSuggestion();
            if (!suggestion.isEmpty()) {
                acceptSuggestion(suggestion);
                return;
            }
        }

        if (event->key() == Qt::Key_Escape) {
            hideAutoComplete();
            return;
        }
    }

    const bool isEnter = (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter);

    if (isEnter && !autoCompletePopup->isVisible()) {
        hideAutoComplete();
        const QString command = getCurrentCommand();
        moveCursor(QTextCursor::End);
        append(QString());
        emit commandEntered(command);
        return;
    }

    if (!autoCompletePopup->isVisible()) {
        if (event->key() == Qt::Key_Up) {
            emit upPressed();
            return;
        }
        if (event->key() == Qt::Key_Down) {
            emit downPressed();
            return;
        }
    }

    QTextCursor cursor = textCursor();
    if (cursor.position() < promptPosition) {
        if (event->key() == Qt::Key_Backspace ||
            event->key() == Qt::Key_Delete ||
            event->key() == Qt::Key_Left) {
            return;
        }
        moveCursor(QTextCursor::End);
    }

    if (event->key() == Qt::Key_Backspace && cursor.position() <= promptPosition) {
        return;
    }

    QTextEdit::keyPressEvent(event);

    if (!event->text().isEmpty() && !isEnter && event->key() != Qt::Key_Escape && event->key() != Qt::Key_Backspace) {
        emit textChangedForAutoComplete();
    } else if (event->key() == Qt::Key_Backspace) {
        emit textChangedForAutoComplete();
    }
}

void TerminalTextEdit::mousePressEvent(QMouseEvent *event)
{
    QTextEdit::mousePressEvent(event);
    ensureCursorInEditableArea();
}

void TerminalTextEdit::mouseDoubleClickEvent(QMouseEvent *event)
{
    QTextEdit::mouseDoubleClickEvent(event);
    ensureCursorInEditableArea();
}

void TerminalTextEdit::ensureCursorInEditableArea()
{
    QTextCursor cursor = textCursor();
    if (cursor.position() < promptPosition) {
        cursor.setPosition(promptPosition);
        setTextCursor(cursor);
    }
}

void TerminalTextEdit::showAutoComplete(const QStringList &suggestions)
{
    if (suggestions.isEmpty()) {
        hideAutoComplete();
        return;
    }

    const QRect cursorRectangle = cursorRect(textCursor());
    const QPoint globalPos = mapToGlobal(cursorRectangle.bottomLeft());
    autoCompletePopup->showSuggestions(suggestions, globalPos);
}

void TerminalTextEdit::hideAutoComplete()
{
    autoCompletePopup->hide();
}

void TerminalTextEdit::acceptSuggestion(const QString &suggestion)
{
    const QString cmd = getCurrentCommand();
    const QStringList parts = cmd.split(' ', Qt::SkipEmptyParts);

    if (parts.isEmpty()) {
        insertPlainText(suggestion + QLatin1Char(' '));
    } else {
        const QString lastWord = parts.last();
        for (int i = 0; i < lastWord.length(); ++i) {
            QTextCursor cursor = textCursor();
            cursor.deletePreviousChar();
            setTextCursor(cursor);
        }
        insertPlainText(suggestion + QLatin1Char(' '));
    }

    hideAutoComplete();
}
