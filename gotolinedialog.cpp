#include "gotolinedialog.h"
#include "codeeditor.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QIntValidator>
#include <QTextBlock>

GoToLineDialog::GoToLineDialog(CodeEditor *editor, QWidget *parent)
    : QDialog(parent), editor(editor)
{
    setWindowTitle(tr("Go to Line"));
    setModal(true);
    setFixedSize(300, 120);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Line number input
    QHBoxLayout *lineLayout = new QHBoxLayout();
    QLabel *lineLabel = new QLabel(tr("Line number:"), this);
    lineNumberEdit = new QLineEdit(this);
    lineNumberEdit->setValidator(new QIntValidator(1, editor->document()->blockCount(), this));

    lineLayout->addWidget(lineLabel);
    lineLayout->addWidget(lineNumberEdit);
    mainLayout->addLayout(lineLayout);

    // Max line info
    maxLineLabel = new QLabel(tr("Maximum line: %1").arg(editor->document()->blockCount()), this);
    mainLayout->addWidget(maxLineLabel);

    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    goButton = new QPushButton(tr("Go"), this);
    cancelButton = new QPushButton(tr("Cancel"), this);

    buttonLayout->addStretch();
    buttonLayout->addWidget(goButton);
    buttonLayout->addWidget(cancelButton);
    mainLayout->addLayout(buttonLayout);

    // Connections
    connect(goButton, &QPushButton::clicked, this, &GoToLineDialog::goToLine);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(lineNumberEdit, &QLineEdit::textChanged, this, &GoToLineDialog::validateLineNumber);
    connect(lineNumberEdit, &QLineEdit::returnPressed, this, &GoToLineDialog::goToLine);

    // Set focus to line edit
    lineNumberEdit->setFocus();
}

void GoToLineDialog::goToLine()
{
    bool ok;
    int lineNumber = lineNumberEdit->text().toInt(&ok);
    if (ok && lineNumber >= 1 && lineNumber <= editor->document()->blockCount()) {
        // Get the block at the specified line
        QTextBlock block = editor->document()->findBlockByLineNumber(lineNumber - 1);
        if (block.isValid()) {
            // Move cursor to the beginning of the line
            QTextCursor cursor = editor->textCursor();
            cursor.setPosition(block.position());
            editor->setTextCursor(cursor);
            editor->centerCursor();
            accept();
        }
    }
}

void GoToLineDialog::validateLineNumber()
{
    bool ok;
    int lineNumber = lineNumberEdit->text().toInt(&ok);
    goButton->setEnabled(ok && lineNumber >= 1 && lineNumber <= editor->document()->blockCount());
}