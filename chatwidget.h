#ifndef CHATWIDGET_H
#define CHATWIDGET_H

#include <QWidget>
#include <QList>
#include <QPair>

class QTextEdit;
class QLineEdit;
class QPushButton;
class QNetworkAccessManager;

class ChatWidget : public QWidget {
    Q_OBJECT
public:
    explicit ChatWidget(QWidget *parent = nullptr);

    // Set the project directory and load its chat history
    void setProjectDirectory(const QString &projectDir);
    QString projectDirectory() const { return m_projectDir; }

    // Save chat history to project's .editerako folder
    void saveChatHistory();
    // Load chat history from project's .editerako folder
    void loadChatHistory();
    // Clear current conversation view and history
    void clearChat();

public slots:
    void sendMessage();

private:
    QTextEdit *conversationView;
    QLineEdit *inputLine;
    QPushButton *sendButton;
    QNetworkAccessManager *networkManager;

    QString m_projectDir;
    // Store messages as pairs (sender, text) for persistence
    QList<QPair<QString, QString>> m_chatHistory;

    void appendMessage(const QString &who, const QString &text, bool addToHistory = true);
    void callGeminiApi(const QString &prompt);
    QString chatHistoryFilePath() const;
};

#endif // CHATWIDGET_H
