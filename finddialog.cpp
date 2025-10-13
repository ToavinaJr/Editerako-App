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
#include <QFrame>

FindReplaceDialog::FindReplaceDialog(CodeEditor *editor, QWidget *parent)
    : QDialog(parent), editor(editor)
{
    setWindowTitle("Find / Replace");
    setMinimumWidth(500);
    setMinimumHeight(280);

    // Style global du dialog
    setStyleSheet(
        "QDialog {"
        "    background-color: #1e1e1e;"
        "    color: #cccccc;"
        "}"
        "QLabel {"
        "    color: #cccccc;"
        "    font-size: 12px;"
        "    padding: 4px 0px;"
        "}"
        "QLineEdit {"
        "    background-color: #3e3e42;"
        "    border: 1px solid #6f6f6f;"
        "    border-radius: 4px;"
        "    color: #cccccc;"
        "    padding: 8px;"
        "    font-size: 12px;"
        "    selection-background-color: #264f78;"
        "}"
        "QLineEdit:focus {"
        "    border: 1px solid #98c379;"
        "}"
        "QCheckBox {"
        "    color: #cccccc;"
        "    font-size: 11px;"
        "    spacing: 8px;"
        "}"
        "QCheckBox::indicator {"
        "    width: 16px;"
        "    height: 16px;"
        "    border: 1px solid #6f6f6f;"
        "    border-radius: 3px;"
        "    background-color: #3e3e42;"
        "}"
        "QCheckBox::indicator:checked {"
        "    background-color: #98c379;"
        "    border-color: #98c379;"
        "}"
        "QPushButton {"
        "    background-color: #3e3e42;"
        "    border: 1px solid #6f6f6f;"
        "    border-radius: 4px;"
        "    color: #cccccc;"
        "    padding: 8px 16px;"
        "    font-size: 11px;"
        "    min-width: 80px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #6f6f6f;"
        "    border-color: #8f8f8f;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #2d2d30;"
        "}"
        "QPushButton#findNextButton {"
        "    background-color: #98c379;"
        "    color: #1e1e1e;"
        "    font-weight: bold;"
        "    border: none;"
        "}"
        "QPushButton#findNextButton:hover {"
        "    background-color: #a8d389;"
        "}"
        "QPushButton#replaceAllButton {"
        "    background-color: #e5c07b;"
        "    color: #1e1e1e;"
        "    font-weight: bold;"
        "    border: none;"
        "}"
        "QPushButton#replaceAllButton:hover {"
        "    background-color: #f0d08b;"
        "}"
        "QFrame#separator {"
        "    background-color: #3e3e42;"
        "    max-height: 1px;"
        "}"
        );

    // Widgets
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
    findLabel->setStyleSheet("font-weight: bold; font-size: 13px;");
    mainLayout->addWidget(findLabel);
    mainLayout->addWidget(searchLineEdit);

    // Section Replace
    mainLayout->addSpacing(8);
    QLabel *replaceLabel = new QLabel("Replace:");
    replaceLabel->setStyleSheet("font-weight: bold; font-size: 13px;");
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
        searchLineEdit->setStyleSheet(
            "QLineEdit {"
            "    border: 2px solid #e06c75;"
            "    background-color: #3e3e42;"
            "    color: #cccccc;"
            "    padding: 8px;"
            "}"
            );
        return;
    }

    // Reset style
    searchLineEdit->setStyleSheet("");

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
        msgBox.setStyleSheet(
            "QMessageBox {"
            "    background-color: #1e1e1e;"
            "    color: #cccccc;"
            "}"
            "QPushButton {"
            "    background-color: #3e3e42;"
            "    border: 1px solid #6f6f6f;"
            "    border-radius: 4px;"
            "    color: #cccccc;"
            "    padding: 6px 16px;"
            "    min-width: 60px;"
            "}"
            "QPushButton:hover {"
            "    background-color: #6f6f6f;"
            "}"
            );
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
    msgBox.setStyleSheet(
        "QMessageBox {"
        "    background-color: #1e1e1e;"
        "    color: #cccccc;"
        "}"
        "QPushButton {"
        "    background-color: #3e3e42;"
        "    border: 1px solid #6f6f6f;"
        "    border-radius: 4px;"
        "    color: #cccccc;"
        "    padding: 6px 16px;"
        "    min-width: 60px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #6f6f6f;"
        "}"
        );
    msgBox.exec();
}
