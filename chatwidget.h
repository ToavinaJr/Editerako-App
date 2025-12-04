#ifndef CHATWIDGET_H
#define CHATWIDGET_H

#include <QWidget>

class QTextEdit;
class QLineEdit;
class QPushButton;
class QNetworkAccessManager;

class ChatWidget : public QWidget {
    Q_OBJECT
public:
    explicit ChatWidget(QWidget *parent = nullptr);

public slots:
    void sendMessage();

private:
    QTextEdit *conversationView;
    QLineEdit *inputLine;
    QPushButton *sendButton;
    QNetworkAccessManager *networkManager;

    void appendMessage(const QString &who, const QString &text);
    void callGeminiApi(const QString &prompt);
};

#endif // CHATWIDGET_H
