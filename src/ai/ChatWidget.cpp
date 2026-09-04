#include "ai/ChatWidget.h"

#include "ai/AccountChatView.h"
#include "ai/AiCatalog.h"
#include "ai/AiProvider.h"
#include "core/AppSettings.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollBar>
#include <QSizePolicy>
#include <QStackedWidget>
#include <QTextDocument>
#include <QTextEdit>
#include <QVBoxLayout>

ChatWidget::ChatWidget(QWidget *parent)
    : QWidget(parent)
{
    m_serviceCombo = new QComboBox(this);
    m_serviceCombo->setObjectName(QStringLiteral("chatServiceCombo"));
    for (const AiService &service : aiServices()) {
        const QString label = service.kind == AiService::Kind::Account
            ? tr("Sign in — %1").arg(service.name)
            : tr("API — %1").arg(service.name);
        m_serviceCombo->addItem(label, service.id);
    }

    m_browserButton = new QPushButton(tr("Sign in"), this);
    m_browserButton->setObjectName(QStringLiteral("chatSignInButton"));
    m_newChatButton = new QPushButton(tr("New chat"), this);
    m_newChatButton->setObjectName(QStringLiteral("chatNewButton"));

    auto *toolbar = new QHBoxLayout;
    toolbar->addWidget(m_serviceCombo, 1);
    toolbar->addWidget(m_browserButton);
    toolbar->addWidget(m_newChatButton);

    m_stack = new QStackedWidget(this);
    m_accountView = new AccountChatView(this);
    m_stack->addWidget(m_accountView);

    m_apiPage = new QWidget(this);
    conversationView = new QTextEdit(m_apiPage);
    conversationView->setReadOnly(true);
    conversationView->setObjectName(QStringLiteral("chatConversation"));
    conversationView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    inputLine = new QLineEdit(m_apiPage);
    inputLine->setPlaceholderText(tr("Ask a question…"));
    inputLine->setObjectName(QStringLiteral("chatInput"));

    sendButton = new QPushButton(tr("➤"), m_apiPage);
    sendButton->setObjectName(QStringLiteral("chatSendButton"));
    sendButton->setFixedSize(44, 44);
    sendButton->setCursor(Qt::PointingHandCursor);

    QWidget *inputContainer = new QWidget(m_apiPage);
    auto *inputLayout = new QHBoxLayout(inputContainer);
    inputLayout->setContentsMargins(0, 0, 0, 0);
    inputLayout->setSpacing(12);
    inputLayout->addWidget(inputLine);
    inputLayout->addWidget(sendButton);

    auto *apiLayout = new QVBoxLayout(m_apiPage);
    apiLayout->setContentsMargins(0, 0, 0, 0);
    apiLayout->addWidget(conversationView, 1);
    apiLayout->addWidget(inputContainer);
    m_stack->addWidget(m_apiPage);

    auto *main = new QVBoxLayout(this);
    main->setContentsMargins(12, 12, 12, 12);
    main->setSpacing(8);
    main->addLayout(toolbar);
    main->addWidget(m_stack, 1);

    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    setMinimumWidth(160);
    setMaximumWidth(520);

    connect(sendButton, &QPushButton::clicked, this, &ChatWidget::sendMessage);
    connect(inputLine, &QLineEdit::returnPressed, this, &ChatWidget::sendMessage);
    connect(m_serviceCombo, &QComboBox::currentIndexChanged, this, &ChatWidget::onServiceChanged);
    connect(m_browserButton, &QPushButton::clicked, m_accountView,
            &AccountChatView::openInSystemBrowser);
    connect(m_newChatButton, &QPushButton::clicked, this, &ChatWidget::newChat);

    reloadFromSettings();
}

ChatWidget::~ChatWidget() = default;

void ChatWidget::reloadFromSettings()
{
    const QString id = AppSettings().aiProvider();
    const int index = m_serviceCombo->findData(id.isEmpty() ? defaultAiProviderId() : id);
    const QSignalBlocker blocker(m_serviceCombo);
    m_serviceCombo->setCurrentIndex(index >= 0 ? index : 0);
    applyService(m_serviceCombo->currentData().toString());
}

void ChatWidget::onServiceChanged()
{
    const QString id = m_serviceCombo->currentData().toString();
    AppSettings().setAiProvider(id);
    applyService(id);
}

void ChatWidget::applyService(const QString &id)
{
    const AiService service = aiServiceById(id);
    const bool account = service.kind == AiService::Kind::Account;
    m_stack->setCurrentWidget(account ? static_cast<QWidget *>(m_accountView) : m_apiPage);
    m_browserButton->setVisible(account);
    m_browserButton->setText(account ? tr("Sign in") : tr("Open"));

    if (account) {
        if (m_provider) {
            m_provider->cancel();
            m_provider->deleteLater();
            m_provider = nullptr;
        }
        m_accountView->navigate(service.accountUrl);
        return;
    }

    if (m_provider && m_provider->providerId() == service.id) {
        return;
    }
    if (m_provider) {
        m_provider->cancel();
        m_provider->deleteLater();
        m_provider = nullptr;
    }
    m_provider = AiProvider::create(service.id, this);
    bindProvider();
}

void ChatWidget::bindProvider()
{
    if (!m_provider) {
        return;
    }
    connect(m_provider, &AiProvider::responseReady, this, &ChatWidget::onProviderResponse);
    connect(m_provider, &AiProvider::errorOccurred, this, &ChatWidget::onProviderError);
}

void ChatWidget::newChat()
{
    const AiService service = aiServiceById(m_serviceCombo->currentData().toString());
    if (service.kind == AiService::Kind::Account) {
        m_accountView->navigate(service.accountUrl);
        return;
    }
    if (m_provider) {
        m_provider->cancel();
    }
    clearChat();
}

void ChatWidget::appendMessage(const QString &who, const QString &text, bool addToHistory)
{
    if (addToHistory) {
        m_chatHistory.append(ChatMessage{who, text});
    }

    const QString escapedText = text.toHtmlEscaped().replace(QStringLiteral("\n"), QStringLiteral("<br>"));
    QString renderedMarkdownHtml;
    {
        QTextDocument mdDoc;
        mdDoc.setMarkdown(text);
        renderedMarkdownHtml = mdDoc.toHtml();
    }

    QString html;
    const QString you = tr("You");
    const QString system = tr("System");
    if (who == you) {
        html = QStringLiteral(
                   "<div style='text-align: right; margin: 12px 0;'>"
                   "<div style='color: #b8b8c0; font-size: 10px; margin-bottom: 4px; margin-right: 8px;'>"
                   "</div>"
                   "<span style='color: white; padding: 14px 18px; border-radius: 20px 20px 4px 20px;"
                   "display: inline-block; max-width: 75%%; text-align: left; font-size: 14px;"
                   "line-height: 1.5; font-weight: 500;'>%1</span></div>")
                   .arg(escapedText);
    } else if (who == system) {
        html = QStringLiteral(
                   "<div style='text-align: center; margin: 16px 0;'>"
                   "<span style='background-color: #3a2a2a; color: #f48771; padding: 10px 16px;"
                   "border-radius: 16px; border: 1px solid #f48771; display: inline-block;"
                   "font-size: 12px; font-weight: 500;'>⚠️ %1</span></div>")
                   .arg(escapedText);
    } else {
        html = QStringLiteral(
                   "<div style='text-align: left; margin: 12px 0;'>"
                   "<div style='color: #8ab4f8; font-size: 11px; font-weight: 600; margin-bottom: 6px;"
                   "margin-left: 6px; letter-spacing: 0.5px;'>✨ %1</div>"
                   "<span style='color: #e8e8e8; padding: 14px 18px; border-radius: 20px 20px 20px 4px;"
                   "border-left: 3px solid #8ab4f8; display: inline-block; max-width: 80%%;"
                   "text-align: left; font-size: 14px; line-height: 1.6;'>%2</span></div>")
                   .arg(who.toHtmlEscaped(), renderedMarkdownHtml);
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
    if (m_provider->isBusy()) {
        return;
    }

    appendMessage(tr("You"), text);
    m_repository.append(ChatMessage{tr("You"), text});
    inputLine->clear();
    m_provider->send(m_context.buildPrompt(text, m_chatHistory));
}

void ChatWidget::onProviderResponse(const QString &text)
{
    const QString who = m_provider ? m_provider->displayName() : tr("Assistant");
    appendMessage(who, text);
    m_repository.append(ChatMessage{who, text});
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
