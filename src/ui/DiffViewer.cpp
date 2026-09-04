#include "ui/DiffViewer.h"

#include "scm/TextDiff.h"

#include <QFontDatabase>
#include <QLabel>
#include <QPlainTextEdit>
#include <QTextBlock>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QVBoxLayout>

DiffViewer::DiffViewer(QWidget *parent)
    : QWidget(parent), m_title(new QLabel(this)), m_text(new QPlainTextEdit(this))
{
    setObjectName(QStringLiteral("diffViewer"));
    m_text->setObjectName(QStringLiteral("diffText"));
    m_text->setReadOnly(true);
    m_text->setLineWrapMode(QPlainTextEdit::NoWrap);
    m_text->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->addWidget(m_title);
    layout->addWidget(m_text);
}

void DiffViewer::setDiff(const QString &path, const QString &text)
{
    m_title->setText(path);
    m_text->setPlainText(text.isEmpty() ? tr("No textual differences.") : text);
    QTextCursor cursor(m_text->document());
    for (QTextBlock block = m_text->document()->begin(); block.isValid(); block = block.next()) {
        const QString line = block.text();
        QColor color;
        if (line.startsWith(QLatin1Char('+')) && !line.startsWith(QStringLiteral("+++"))) color = QColor(QStringLiteral("#2ea043"));
        else if (line.startsWith(QLatin1Char('-')) && !line.startsWith(QStringLiteral("---"))) color = QColor(QStringLiteral("#f85149"));
        else if (line.startsWith(QStringLiteral("@@"))) color = QColor(QStringLiteral("#58a6ff"));
        if (!color.isValid()) continue;
        cursor.setPosition(block.position());
        cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        QTextCharFormat format;
        format.setForeground(color);
        cursor.mergeCharFormat(format);
    }
    m_text->moveCursor(QTextCursor::Start);
}

void DiffViewer::setTexts(const QString &title, const QString &left, const QString &right,
                          const QString &leftName, const QString &rightName)
{
    setDiff(title, TextDiff::unified(left, right, leftName, rightName));
}

