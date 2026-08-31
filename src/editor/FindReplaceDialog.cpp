#include "editor/FindReplaceDialog.h"
#include "editor/CodeEditor.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QTextCursor>
#include <QRegularExpression>
#include <QMessageBox>
#include <QFrame>
#include <QStyle>

FindReplaceDialog::FindReplaceDialog(CodeEditor *editor, QWidget *parent)
    : QDialog(parent), editor(editor)
{
    setWindowTitle("Find / Replace");
    setMinimumWidth(500);
    setMinimumHeight(280);
    setObjectName(QStringLiteral("FindReplaceDialog"));
    searchLineEdit = new QLineEdit;
    searchLineEdit->setPlaceholderText("Search text...");

    replaceLineEdit = new QLineEdit;
    replaceLineEdit->setPlaceholderText("Replace with...");

    caseSensitiveCheckBox = new QCheckBox("Case sensitive");
    regexCheckBox = new QCheckBox("Use Regular Expression");

    findNextButton = new QPushButton("Find Next");
    findNextButton->setObjectName("findNextButton");

    replaceButton = new QPushButton("Replace");
    replaceAllButton = new QPushButton("Replace All");
    replaceAllButton->setObjectName("replaceAllButton");

    cancelButton = new QPushButton("Cancel");

    // Connexions
    connect(findNextButton, &QPushButton::clicked, this, &FindReplaceDialog::findNext);
    connect(replaceButton, &QPushButton::clicked, this, &FindReplaceDialog::replace);
    connect(replaceAllButton, &QPushButton::clicked, this, &FindReplaceDialog::replaceAll);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::close);

    // Layout principal
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // Section Find
    QLabel *findLabel = new QLabel("Find:");
    findLabel->setObjectName(QStringLiteral("sectionLabel"));
    mainLayout->addWidget(findLabel);
    mainLayout->addWidget(searchLineEdit);

    // Section Replace
    mainLayout->addSpacing(8);
    QLabel *replaceLabel = new QLabel("Replace:");
    replaceLabel->setObjectName(QStringLiteral("sectionLabel"));
    mainLayout->addWidget(replaceLabel);
    mainLayout->addWidget(replaceLineEdit);

    // Séparateur
    mainLayout->addSpacing(12);
    QFrame *separator = new QFrame;
    separator->setObjectName("separator");
    separator->setFrameShape(QFrame::HLine);
    mainLayout->addWidget(separator);
    mainLayout->addSpacing(8);

    // Options
    QHBoxLayout *optionsLayout = new QHBoxLayout;
    optionsLayout->setSpacing(20);
    optionsLayout->addWidget(caseSensitiveCheckBox);
    optionsLayout->addWidget(regexCheckBox);
    optionsLayout->addStretch();
    mainLayout->addLayout(optionsLayout);

    // Boutons d'action
    mainLayout->addSpacing(16);
    QHBoxLayout *buttonsLayout = new QHBoxLayout;
    buttonsLayout->setSpacing(10);
    buttonsLayout->addWidget(findNextButton);
    buttonsLayout->addWidget(replaceButton);
    buttonsLayout->addWidget(replaceAllButton);
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(cancelButton);
    mainLayout->addLayout(buttonsLayout);

    setLayout(mainLayout);

    // Focus sur le champ de recherche au démarrage
    searchLineEdit->setFocus();
}

void FindReplaceDialog::findNext()
{
    QString pattern = searchLineEdit->text();
    if(pattern.isEmpty()) {
        searchLineEdit->setProperty("invalid", true);
        searchLineEdit->style()->unpolish(searchLineEdit);
        searchLineEdit->style()->polish(searchLineEdit);
        return;
    }

    searchLineEdit->setProperty("invalid", false);
    searchLineEdit->style()->unpolish(searchLineEdit);
    searchLineEdit->style()->polish(searchLineEdit);

    QTextDocument::FindFlags flags;
    if(caseSensitiveCheckBox->isChecked())
        flags |= QTextDocument::FindCaseSensitively;

    QTextCursor cursor = editor->textCursor();
    bool found = false;

    if(regexCheckBox->isChecked()) {
        QRegularExpression regex(pattern,
                                 caseSensitiveCheckBox->isChecked()
                                     ? QRegularExpression::NoPatternOption
                                     : QRegularExpression::CaseInsensitiveOption);

        QRegularExpressionMatch match = regex.match(editor->toPlainText(), cursor.position());
        if(match.hasMatch()) {
            cursor.setPosition(match.capturedStart());
            cursor.setPosition(match.capturedEnd(), QTextCursor::KeepAnchor);
            editor->setTextCursor(cursor);
            found = true;
        }
    } else {
        int index = editor->toPlainText().indexOf(
            pattern,
            cursor.position(),
            caseSensitiveCheckBox->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive
            );
        if(index != -1) {
            cursor.setPosition(index);
            cursor.setPosition(index + pattern.length(), QTextCursor::KeepAnchor);
            editor->setTextCursor(cursor);
            found = true;
        }
    }

    if(!found) {
        QMessageBox msgBox(this);
        msgBox.setWindowTitle("Find");
        msgBox.setText("No more matches found.");
        msgBox.setIcon(QMessageBox::Information);
        msgBox.exec();
    }
}

void FindReplaceDialog::replace()
{
    QTextCursor cursor = editor->textCursor();
    if(cursor.hasSelection()) {
        cursor.insertText(replaceLineEdit->text());
    }
    findNext();
}

void FindReplaceDialog::replaceAll()
{
    QString text = editor->toPlainText();
    QString pattern = searchLineEdit->text();
    if(pattern.isEmpty()) return;

    int count = 0;

    if(regexCheckBox->isChecked()) {
        QRegularExpression regex(pattern,
                                 caseSensitiveCheckBox->isChecked()
                                     ? QRegularExpression::NoPatternOption
                                     : QRegularExpression::CaseInsensitiveOption);

        QRegularExpressionMatchIterator it = regex.globalMatch(text);
        while(it.hasNext()) {
            it.next();
            count++;
        }
        text.replace(regex, replaceLineEdit->text());
    } else {
        Qt::CaseSensitivity cs = caseSensitiveCheckBox->isChecked()
        ? Qt::CaseSensitive
        : Qt::CaseInsensitive;
        count = text.count(pattern, cs);
        text.replace(pattern, replaceLineEdit->text(), cs);
    }

    editor->setPlainText(text);

    // Message de confirmation
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Replace All");
    msgBox.setText(QString("Replaced %1 occurrence(s).").arg(count));
    msgBox.setIcon(QMessageBox::Information);
    msgBox.exec();
}
