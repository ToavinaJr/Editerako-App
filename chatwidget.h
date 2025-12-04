#ifndef CHATWIDGET_H
#define CHATWIDGET_H

#include <QWidget>
#include <QList>
#include <QPair>
#include <QSqlDatabase>

class QTextEdit;
class QLineEdit;
class QPushButton;
class QNetworkAccessManager;

class ChatWidget : public QWidget {
    Q_OBJECT
public:
    explicit ChatWidget(QWidget *parent = nullptr);
    ~ChatWidget();

    // Set the project directory and load its chat history
    void setProjectDirectory(const QString &projectDir);
    QString projectDirectory() const { return m_projectDir; }

    // Save chat history to SQLite database
    void saveChatHistory();
    // Load chat history from SQLite database
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
    QSqlDatabase m_db;
    QString m_dbConnectionName;
    
    // Store messages as pairs (sender, text) for persistence
    QList<QPair<QString, QString>> m_chatHistory;

    void appendMessage(const QString &who, const QString &text, bool addToHistory = true);
    void callGeminiApi(const QString &prompt);
    QString databaseFilePath() const;
    void initDatabase();
    void closeDatabase();
    void saveMessageToDb(const QString &sender, const QString &text);
};

#endif // CHATWIDGET_H
