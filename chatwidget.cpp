#include "chatwidget.h"
#include "ai/AiProvider.h"

#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollBar>
#include <QSizePolicy>
#include <QSplitter>
#include <QTextDocument>
#include <QTextEdit>
#include <QVBoxLayout>

ChatWidget::ChatWidget(QWidget *parent)
    : QWidget(parent)
    , conversationView(new QTextEdit(this))
    , inputLine(new QLineEdit(this))
    , sendButton(new QPushButton(tr("➤"), this))
    , m_provider(AiProvider::create(this))
{
    conversationView->setReadOnly(true);
    conversationView->setObjectName(QStringLiteral("chatConversation"));
    conversationView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    inputLine->setPlaceholderText(tr("Posez votre question à Gemini..."));
    inputLine->setObjectName(QStringLiteral("chatInput"));
    inputLine->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    sendButton->setObjectName(QStringLiteral("chatSendButton"));
    sendButton->setFixedSize(44, 44);
    sendButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    sendButton->setCursor(Qt::PointingHandCursor);

    QWidget *inputContainer = new QWidget(this);
    QHBoxLayout *inputLayout = new QHBoxLayout(inputContainer);
    inputLayout->setContentsMargins(0, 0, 0, 0);
    inputLayout->setSpacing(12);
    inputLayout->addWidget(inputLine);
    inputLayout->addWidget(sendButton);

    QSplitter *split = new QSplitter(Qt::Vertical, this);
    split->addWidget(conversationView);
    split->addWidget(inputContainer);
    split->setStretchFactor(0, 1);
    split->setCollapsible(0, false);
    split->setCollapsible(1, false);

    QVBoxLayout *main = new QVBoxLayout(this);
    main->setContentsMargins(16, 16, 16, 16);
    main->setSpacing(12);
    main->addWidget(split);
    main->setStretch(0, 1);

    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    setMinimumWidth(160);
    setMaximumWidth(520);

    connect(sendButton, &QPushButton::clicked, this, &ChatWidget::sendMessage);
    connect(inputLine, &QLineEdit::returnPressed, this, &ChatWidget::sendMessage);
    connect(m_provider, &AiProvider::responseReady, this, &ChatWidget::onProviderResponse);
    connect(m_provider, &AiProvider::errorOccurred, this, &ChatWidget::onProviderError);
}

ChatWidget::~ChatWidget() = default;

void ChatWidget::appendMessage(const QString &who, const QString &text, bool addToHistory)
{
    if (addToHistory) {
        m_chatHistory.append(ChatMessage{who, text});
    }

    QString html;
    const QString escapedText = text.toHtmlEscaped().replace(QStringLiteral("\n"), QStringLiteral("<br>"));
    QString renderedMarkdownHtml;
    {
        QTextDocument mdDoc;
        mdDoc.setMarkdown(text);
        renderedMarkdownHtml = mdDoc.toHtml();
    }

    if (who == tr("You")) {
        html = QString(
            "<div style='text-align: right; margin: 12px 0;'>"
            "<div style='"
            "color: #b8b8c0;"
            "font-size: 10px;"
            "margin-bottom: 4px;"
            "margin-right: 8px;"
            "<span style='"
            "color: white;"
            "padding: 14px 18px;"
            "border-radius: 20px 20px 4px 20px;"
            "display: inline-block;"
            "max-width: 75%%;"
            "text-align: left;"
            "font-size: 14px;"
            "line-height: 1.5;"
            "font-weight: 500;"
            "'>%1</span>"
            "</div>"
        ).arg(escapedText);
    } else if (who == tr("Gemini")) {
        html = QString(
            "<div style='text-align: left; margin: 12px 0;'>"
            "<div style='"
            "color: #8ab4f8;"
            "font-size: 11px;"
            "font-weight: 600;"
            "margin-bottom: 6px;"
            "margin-left: 6px;"
            "letter-spacing: 0.5px;"
            "'>✨ GEMINI AI</div>"
            "<span style='"
            "color: #e8e8e8;"
            "padding: 14px 18px;"
            "border-radius: 20px 20px 20px 4px;"
            "border-left: 3px solid #8ab4f8;"
            "display: inline-block;"
            "max-width: 80%%;"
            "text-align: left;"
            "font-size: 14px;"
            "line-height: 1.6;"
            "'>%1</span>"
            "</div>"
        ).arg(renderedMarkdownHtml);
    } else {
        html = QString(
            "<div style='text-align: center; margin: 16px 0;'>"
            "<span style='"
            "background-color: #3a2a2a;"
            "color: #f48771;"
            "padding: 10px 16px;"
            "border-radius: 16px;"
            "border: 1px solid #f48771;"
            "display: inline-block;"
            "font-size: 12px;"
            "font-weight: 500;"
            "'>⚠️ %1</span>"
            "</div>"
        ).arg(escapedText);
    }

    conversationView->append(html);
    QScrollBar *sb = conversationView->verticalScrollBar();
    if (sb) {
        sb->setValue(sb->maximum());
    }
}

void ChatWidget::sendMessage()
{
    const QString text = inputLine->text().trimmed();
    if (text.isEmpty() || !m_provider) {
        return;
    }

    appendMessage(tr("You"), text);
    m_repository.append(ChatMessage{tr("You"), text});
    inputLine->clear();

    m_provider->send(m_context.buildPrompt(text, m_chatHistory));
}

void ChatWidget::onProviderResponse(const QString &text)
{
    appendMessage(tr("Gemini"), text);
    m_repository.append(ChatMessage{tr("Gemini"), text});
}

void ChatWidget::onProviderError(const QString &message)
{
    appendMessage(tr("System"), tr("Error: %1").arg(message), false);
}

void ChatWidget::setProjectDirectory(const QString &projectDir)
{
    m_projectDir = projectDir;
    clearChat();
    m_repository.open(m_projectDir);
    loadChatHistory();
}

void ChatWidget::setActiveFileContext(const QString &path, const QString &content)
{
    m_context.setActiveFile(path, content);
}

void ChatWidget::saveChatHistory()
{
}

void ChatWidget::loadChatHistory()
{
    const QList<ChatMessage> messages = m_repository.loadAll();
    for (const ChatMessage &message : messages) {
        m_chatHistory.append(message);
        appendMessage(message.sender, message.text, false);
    }
}

void ChatWidget::clearChat()
{
    m_chatHistory.clear();
    conversationView->clear();
}
