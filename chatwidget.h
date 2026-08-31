#ifndef CHATWIDGET_H
#define CHATWIDGET_H

#include "ai/ChatMessage.h"
#include "ai/ChatRepository.h"
#include "ai/ContextBuilder.h"

#include <QList>
#include <QWidget>

class AiProvider;
class QTextEdit;
class QLineEdit;
class QPushButton;

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

public slots:
    void sendMessage();

private:
    void appendMessage(const QString &who, const QString &text, bool addToHistory = true);
    void onProviderResponse(const QString &text);
    void onProviderError(const QString &message);

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
