#include "chatwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QUrl>
#include <QUrlQuery>
#include <QApplication>
#include <QScrollBar>
#include <QDateTime>

ChatWidget::ChatWidget(QWidget *parent)
    : QWidget(parent)
    , conversationView(new QTextEdit(this))
    , inputLine(new QLineEdit(this))
    , sendButton(new QPushButton(tr("➤"), this))
    , networkManager(new QNetworkAccessManager(this))
{
    QString time = QDateTime::currentDateTime().toString("HH:mm");
    
    // Conversation view styling - Design moderne avec dégradé subtil
    conversationView->setReadOnly(true);
    conversationView->setStyleSheet(
        "QTextEdit {"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "    stop:0 #1a1a1d, stop:1 #16161a);"
        "  color: #e8e8e8;"
        "  border: none;"
        "  border-radius: 12px;"
        "  padding: 16px;"
        "  font-family: 'Segoe UI', 'SF Pro Display', system-ui, sans-serif;"
        "  font-size: 14px;"
        "  line-height: 1.6;"
        "  selection-background-color: #4a9eff;"
        "}"
        "QScrollBar:vertical {"
        "  background: transparent;"
        "  width: 10px;"
        "  margin: 4px;"
        "}"
        "QScrollBar::handle:vertical {"
        "  background: rgba(100, 100, 120, 0.4);"
        "  border-radius: 5px;"
        "  min-height: 30px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "  background: rgba(120, 120, 140, 0.6);"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "  height: 0px;"
        "}"
    );

    // Input field styling - Design plus moderne avec ombre
    inputLine->setPlaceholderText(tr("Posez votre question à Gemini..."));
    inputLine->setStyleSheet(
        "QLineEdit {"
        "  background-color: #2a2a2f;"
        "  color: #e8e8e8;"
        "  border: 2px solid transparent;"
        "  border-radius: 24px;"
        "  padding: 12px 20px;"
        "  font-family: 'Segoe UI', 'SF Pro Display', system-ui, sans-serif;"
        "  font-size: 14px;"
        "  selection-background-color: #4a9eff;"
        "}"
        "QLineEdit:focus {"
        "  border: 2px solid #4a9eff;"
        "  background-color: #323238;"
        "}"
        "QLineEdit:hover {"
        "  background-color: #32323a;"
        "}"
        "QLineEdit::placeholder {"
        "  color: #7a7a85;"
        "}"
    );

    // Send button styling - Design gradient moderne
    sendButton->setFixedSize(44, 44);
    sendButton->setCursor(Qt::PointingHandCursor);
    sendButton->setStyleSheet(
        "QPushButton {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:1,"
        "    stop:0 #4a9eff, stop:1 #357abd);"
        "  color: white;"
        "  border: none;"
        "  border-radius: 22px;"
        "  font-size: 16px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:1,"
        "    stop:0 #5fadff, stop:1 #4a8fd4);"
        "}"
        "QPushButton:pressed {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:1,"
        "    stop:0 #3a8eef, stop:1 #2a6aad);"
        "  padding-top: 2px;"
        "}"
    );

    // Input layout avec espacement amélioré
    QHBoxLayout *inputLayout = new QHBoxLayout();
    inputLayout->setContentsMargins(0, 0, 0, 0);
    inputLayout->setSpacing(12);
    inputLayout->addWidget(inputLine);
    inputLayout->addWidget(sendButton);

    // Main layout avec padding optimisé
    QVBoxLayout *main = new QVBoxLayout(this);
    main->setContentsMargins(16, 16, 16, 16);
    main->setSpacing(16);
    main->addWidget(conversationView);
    main->addLayout(inputLayout);

    // Widget background avec dégradé sophistiqué
    setStyleSheet(
        "QWidget {"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "    stop:0 #0f0f12, stop:1 #1a1a1f);"
        "}"
    );

    connect(sendButton, &QPushButton::clicked, this, &ChatWidget::sendMessage);
    connect(inputLine, &QLineEdit::returnPressed, this, &ChatWidget::sendMessage);
}

void ChatWidget::appendMessage(const QString &who, const QString &text)
{
    QString html;
    QString escapedText = text.toHtmlEscaped().replace("\n", "<br>");
    
    if (who == tr("You")) {
        // User message - Design moderne avec ombre et gradient
        html = QString(
            "<div style='text-align: right; margin: 12px 0;'>"
            "<span style='"
            "background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #4a9eff, stop:1 #357abd);"
            "color: white;"
            "padding: 12px 18px;"
            "border-radius: 20px 20px 4px 20px;"
            "display: inline-block;"
            "max-width: 75%%;"
            "text-align: left;"
            "font-size: 14px;"
            "line-height: 1.5;"
            "box-shadow: 0 4px 12px rgba(74, 158, 255, 0.25);"
            "font-weight: 500;"
            "'>%1</span>"
            "</div>"
        ).arg(escapedText);
    } else if (who == tr("Gemini")) {
        // Gemini response - Design élégant avec bordure subtile
        html = QString(
            "<div style='text-align: left; margin: 12px 0;'>"
            "<div style='"
            "color: #8ab4f8;"
            "font-size: 11px;"
            "font-weight: 600;"
            "margin-bottom: 6px;"
            "margin-left: 6px;"
            "letter-spacing: 0.5px;"
            "text-transform: uppercase;"
            "'>✨ Gemini AI</div>"
            "<span style='"
            "background: linear-gradient(135deg, #2a2a32 0%, #25252d 100%);"
            "color: #e8e8e8;"
            "padding: 14px 18px;"
            "border-radius: 20px 20px 20px 4px;"
            "border-left: 3px solid #8ab4f8;"
            "display: inline-block;"
            "max-width: 80%%;"
            "text-align: left;"
            "font-size: 14px;"
            "line-height: 1.6;"
            "box-shadow: 0 2px 8px rgba(0, 0, 0, 0.3);"
            "'>%1</span>"
            "</div>"
        ).arg(escapedText);
    } else {
        // System message - Design discret mais visible
        html = QString(
            "<div style='text-align: center; margin: 16px 0;'>"
            "<span style='"
            "background: rgba(244, 135, 113, 0.15);"
            "color: #f48771;"
            "padding: 10px 16px;"
            "border-radius: 16px;"
            "border: 1px solid rgba(244, 135, 113, 0.3);"
            "display: inline-block;"
            "font-size: 12px;"
            "font-weight: 500;"
            "letter-spacing: 0.3px;"
            "'>⚠️ %1</span>"
            "</div>"
        ).arg(escapedText);
    }
    
    conversationView->append(html);
    QScrollBar *sb = conversationView->verticalScrollBar();
    if (sb) sb->setValue(sb->maximum());
}

void ChatWidget::sendMessage()
{
    QString text = inputLine->text().trimmed();
    if (text.isEmpty()) return;

    appendMessage(tr("You"), text);
    inputLine->clear();

    callGeminiApi(text);
}

void ChatWidget::callGeminiApi(const QString &prompt)
{
    // Build request JSON according to the example REST call
    QJsonObject partObj;
    partObj["text"] = prompt;

    QJsonArray partsArr;
    partsArr.append(partObj);

    QJsonObject contentObj;
    contentObj["parts"] = partsArr;

    QJsonArray contentsArr;
    contentsArr.append(contentObj);

    QJsonObject root;
    root["contents"] = contentsArr;

    QJsonDocument doc(root);
    QByteArray body = doc.toJson();

    // Use gemini-2.0-flash model (more stable/available)
    QUrl url("https://generativelanguage.googleapis.com/v1beta/models/gemini-2.0-flash:generateContent");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // Read API key from environment variable GEMINI_API_KEY
    QByteArray apiKey = qgetenv("GEMINI_API_KEY");
    if (apiKey.isEmpty()) {
        appendMessage(tr("System"), tr("GEMINI_API_KEY not set in environment. Set it and retry."));
        return;
    }

    // The API expects x-goog-api-key header (per example). Set it here.
    request.setRawHeader("x-goog-api-key", apiKey);

    QNetworkReply *reply = networkManager->post(request, body);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        QByteArray resp = reply->readAll();
        
        if (reply->error() != QNetworkReply::NoError) {
            // Show error details including server response body for debugging
            QString errMsg = reply->errorString();
            if (!resp.isEmpty()) {
                errMsg += QString("\nServer response: %1").arg(QString::fromUtf8(resp));
            }
            appendMessage(tr("Gemini"), tr("Error: %1").arg(errMsg));
            reply->deleteLater();
            return;
        }

        // Try to parse response JSON and extract text if available.
        QJsonParseError err;
        QJsonDocument jdoc = QJsonDocument::fromJson(resp, &err);
        QString outText;
        if (err.error == QJsonParseError::NoError && jdoc.isObject()) {
            QJsonObject obj = jdoc.object();
            
            // Structure: { "candidates": [{ "content": { "parts": [{ "text": "..." }] } }] }
            if (obj.contains("candidates") && obj["candidates"].isArray()) {
                QJsonArray cand = obj["candidates"].toArray();
                if (!cand.isEmpty() && cand[0].isObject()) {
                    QJsonObject c0 = cand[0].toObject();
                    if (c0.contains("content") && c0["content"].isObject()) {
                        QJsonObject content = c0["content"].toObject();
                        if (content.contains("parts") && content["parts"].isArray()) {
                            QJsonArray parts = content["parts"].toArray();
                            for (const QJsonValue &partVal : parts) {
                                if (partVal.isObject()) {
                                    QJsonObject part = partVal.toObject();
                                    if (part.contains("text")) {
                                        outText += part["text"].toString();
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // Fallback: raw response
            if (outText.isEmpty()) {
                outText = QString::fromUtf8(resp);
            }
        } else {
            outText = QString::fromUtf8(resp);
        }

        appendMessage(tr("Gemini"), outText);
        reply->deleteLater();
    });
}