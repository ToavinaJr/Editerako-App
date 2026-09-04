#include "ui/OutputPanel.h"

#include <QFontDatabase>
#include <QPlainTextEdit>
#include <QTextCursor>
#include <QVBoxLayout>

OutputPanel::OutputPanel(QWidget *parent)
    : QWidget(parent)
    , m_text(new QPlainTextEdit(this))
{
    setObjectName(QStringLiteral("outputPanel"));
    m_text->setObjectName(QStringLiteral("outputText"));
    m_text->setReadOnly(true);
    m_text->setLineWrapMode(QPlainTextEdit::NoWrap);
    m_text->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->addWidget(m_text);
}

void OutputPanel::clear()
{
    m_text->clear();
}

void OutputPanel::append(const QString &text)
{
    m_text->moveCursor(QTextCursor::End);
    m_text->insertPlainText(text);
    m_text->moveCursor(QTextCursor::End);
}
