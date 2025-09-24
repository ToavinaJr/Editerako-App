#include "finddialog.h"
#include "codeeditor.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QTextCursor>
#include <QRegularExpression>
#include <QMessageBox>

FindReplaceDialog::FindReplaceDialog(CodeEditor *editor, QWidget *parent)
    : QDialog(parent), editor(editor)
{
    setWindowTitle("Find / Replace");

    searchLineEdit = new QLineEdit;
    replaceLineEdit = new QLineEdit;
    caseSensitiveCheckBox = new QCheckBox("Case sensitive");
    regexCheckBox = new QCheckBox("Use Regular Expression");

    findNextButton = new QPushButton("Find Next");
    replaceButton = new QPushButton("Replace");
    replaceAllButton = new QPushButton("Replace All");
    cancelButton = new QPushButton("Cancel");

    connect(findNextButton, &QPushButton::clicked, this, &FindReplaceDialog::findNext);
    connect(replaceButton, &QPushButton::clicked, this, &FindReplaceDialog::replace);
    connect(replaceAllButton, &QPushButton::clicked, this, &FindReplaceDialog::replaceAll);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::close);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(new QLabel("Find:"));
    mainLayout->addWidget(searchLineEdit);
    mainLayout->addWidget(new QLabel("Replace:"));
    mainLayout->addWidget(replaceLineEdit);
    mainLayout->addWidget(caseSensitiveCheckBox);
    mainLayout->addWidget(regexCheckBox);

    QHBoxLayout *buttonsLayout = new QHBoxLayout;
    buttonsLayout->addWidget(findNextButton);
    buttonsLayout->addWidget(replaceButton);
    buttonsLayout->addWidget(replaceAllButton);
    buttonsLayout->addWidget(cancelButton);
    mainLayout->addLayout(buttonsLayout);

    setLayout(mainLayout);
}

void FindReplaceDialog::findNext()
{
    QString pattern = searchLineEdit->text();
    if(pattern.isEmpty()) return;

    QTextDocument::FindFlags flags;
    if(caseSensitiveCheckBox->isChecked())
        flags |= QTextDocument::FindCaseSensitively;

    QTextCursor cursor = editor->textCursor();
    bool found = false;

    if(regexCheckBox->isChecked()) {
        QRegularExpression regex(pattern, caseSensitiveCheckBox->isChecked() ? QRegularExpression::NoPatternOption
                                                                             : QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch match = regex.match(editor->toPlainText(), cursor.position());
        if(match.hasMatch()) {
            cursor.setPosition(match.capturedStart());
            cursor.setPosition(match.capturedEnd(), QTextCursor::KeepAnchor);
            editor->setTextCursor(cursor);
            found = true;
        }
    } else {
        int index = editor->toPlainText().indexOf(pattern, cursor.position(), caseSensitiveCheckBox->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive);
        if(index != -1) {
            cursor.setPosition(index);
            cursor.setPosition(index + pattern.length(), QTextCursor::KeepAnchor);
            editor->setTextCursor(cursor);
            found = true;
        }
    }

    if(!found)
        QMessageBox::information(this, "Find", "No more matches found.");
}

void FindReplaceDialog::replace()
{
    QTextCursor cursor = editor->textCursor();
    if(cursor.hasSelection())
        cursor.insertText(replaceLineEdit->text());
    findNext(); // avancer vers le prochain
}


void FindReplaceDialog::replaceAll()
{
    QString text = editor->toPlainText();
    QString pattern = searchLineEdit->text();
    if(pattern.isEmpty()) return;

    if(regexCheckBox->isChecked()) {
        QRegularExpression regex(pattern, caseSensitiveCheckBox->isChecked() ? QRegularExpression::NoPatternOption
                                                                             : QRegularExpression::CaseInsensitiveOption);
        text.replace(regex, replaceLineEdit->text());
    } else {
        Qt::CaseSensitivity cs = caseSensitiveCheckBox->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive;
        text.replace(pattern, replaceLineEdit->text(), cs);
    }

    editor->setPlainText(text);
}

