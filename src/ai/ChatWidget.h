#ifndef EDITERAKO_CHATWIDGET_H
#define EDITERAKO_CHATWIDGET_H

#include "ai/ChatMessage.h"
#include "ai/ChatRepository.h"
#include "ai/ContextBuilder.h"

#include <QList>
#include <QWidget>

class AccountChatView;
class AiProvider;
class QComboBox;
class QLineEdit;
class QPushButton;
class QStackedWidget;
class QTextEdit;

class ChatWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ChatWidget(QWidget *parent = nullptr);
    ~ChatWidget() override;

    void setProjectDirectory(const QString &projectDir);
    [[nodiscard]] QString projectDirectory() const { return m_projectDir; }
    void setActiveFileContext(const QString &path, const QString &content);

    void saveChatHistory();
    void loadChatHistory();
    void clearChat();
    void reloadFromSettings();

public slots:
    void sendMessage();

private:
    void appendMessage(const QString &who, const QString &text, bool addToHistory = true);
    void onProviderResponse(const QString &text);
    void onProviderError(const QString &message);
    void bindProvider();
    void onServiceChanged();
    void applyService(const QString &id);
    void newChat();

    QComboBox *m_serviceCombo = nullptr;
    QPushButton *m_browserButton = nullptr;
    QPushButton *m_newChatButton = nullptr;
    QStackedWidget *m_stack = nullptr;
    AccountChatView *m_accountView = nullptr;
    QWidget *m_apiPage = nullptr;
    QTextEdit *conversationView = nullptr;
    QLineEdit *inputLine = nullptr;
    QPushButton *sendButton = nullptr;

    QString m_projectDir;
    ChatRepository m_repository;
    ContextBuilder m_context;
    AiProvider *m_provider = nullptr;
    QList<ChatMessage> m_chatHistory;
};

#endif
