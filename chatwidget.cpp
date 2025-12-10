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
#include <QTextDocument>
#include <QSplitter>
#include <QDir>
#include <QFile>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QUuid>

ChatWidget::ChatWidget(QWidget *parent)
    : QWidget(parent)
    , conversationView(new QTextEdit(this))
    , inputLine(new QLineEdit(this))
    , sendButton(new QPushButton(tr("➤"), this))
    , networkManager(new QNetworkAccessManager(this))
    , m_dbConnectionName(QUuid::createUuid().toString())
{
    QString time = QDateTime::currentDateTime().toString("HH:mm");
    
    // Conversation view styling - Design moderne avec dégradé subtil
    conversationView->setReadOnly(true);
    conversationView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
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
    inputLine->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
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
    sendButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
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

    // Input layout encapsulated in a container so it can be placed in a splitter
    QWidget *inputContainer = new QWidget(this);
    QHBoxLayout *inputLayout = new QHBoxLayout(inputContainer);
    inputLayout->setContentsMargins(0, 0, 0, 0);
    inputLayout->setSpacing(12);
    inputLayout->addWidget(inputLine);
    inputLayout->addWidget(sendButton);

    // Use a vertical splitter so the user can stretch the conversation area (height)
    QSplitter *split = new QSplitter(Qt::Vertical, this);
    split->addWidget(conversationView);
    split->addWidget(inputContainer);
    split->setStretchFactor(0, 1);
    split->setCollapsible(0, false);
    split->setCollapsible(1, false);

    // Main layout with padding optimized
    QVBoxLayout *main = new QVBoxLayout(this);
    main->setContentsMargins(16, 16, 16, 16);
    main->setSpacing(12);
    main->addWidget(split);

    // Ensure conversationView expands to fill available space inside splitter
    main->setStretch(0, 1);

    // Allow the ChatWidget itself to be stretched by parent layouts/splitter
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    // Reduce default minimum width so the chat column is narrower on small screens
    setMinimumWidth(160);
    // Optional: cap maximum width to avoid overly large chat column
    setMaximumWidth(520);

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

void ChatWidget::appendMessage(const QString &who, const QString &text, bool addToHistory)
{
    // Store in history if requested
    if (addToHistory) {
        m_chatHistory.append(qMakePair(who, text));
    }
    
    QString html;
    QString escapedText = text.toHtmlEscaped().replace("\n", "<br>");
    // Render Markdown to HTML for model responses so formatting is preserved
    QString renderedMarkdownHtml;
    {
        QTextDocument mdDoc;
        mdDoc.setMarkdown(text);
        renderedMarkdownHtml = mdDoc.toHtml();
    }
    
    if (who == tr("You")) {
        // User message - Design moderne avec fond violet solide
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
        // Gemini response - Design élégant avec fond gris foncé solide
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
        // System message - Design discret mais visible
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
    if (sb) sb->setValue(sb->maximum());
}

void ChatWidget::sendMessage()
{
    QString text = inputLine->text().trimmed();
    if (text.isEmpty()) return;

    appendMessage(tr("You"), text);
    // Save user message to database
    saveMessageToDb(tr("You"), text);
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

    // Use gemini-1.5-flash model (more stable/available)
    QUrl url("https://generativelanguage.googleapis.com/v1beta/models/gemini-2.0-flash-001:generateContent");
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
        // Save message to database immediately
        saveMessageToDb(tr("Gemini"), outText);
        reply->deleteLater();
    });
}

ChatWidget::~ChatWidget()
{
    closeDatabase();
}

void ChatWidget::setProjectDirectory(const QString &projectDir)
{
    // Close previous database connection
    closeDatabase();
    
    m_projectDir = projectDir;
    
    // Clear current view and load new project's history
    clearChat();
    
    // Initialize database for new project and load history
    initDatabase();
    loadChatHistory();
}

QString ChatWidget::databaseFilePath() const
{
    if (m_projectDir.isEmpty()) return QString();
    return QDir(m_projectDir).filePath(".editerako/chat_history.db");
}

void ChatWidget::initDatabase()
{
    if (m_projectDir.isEmpty()) return;
    
    // Ensure .editerako directory exists
    QDir dir(m_projectDir);
    if (!dir.exists(".editerako")) {
        dir.mkdir(".editerako");
    }
    
    // Create database connection with unique name
    m_db = QSqlDatabase::addDatabase("QSQLITE", m_dbConnectionName);
    m_db.setDatabaseName(databaseFilePath());
    
    if (!m_db.open()) {
        qWarning() << "Failed to open chat history database:" << m_db.lastError().text();
        return;
    }
    
    // Create table if not exists
    QSqlQuery query(m_db);
    query.exec(
        "CREATE TABLE IF NOT EXISTS chat_messages ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  sender TEXT NOT NULL,"
        "  message TEXT NOT NULL,"
        "  timestamp DATETIME DEFAULT CURRENT_TIMESTAMP"
        ")"
    );
}

void ChatWidget::closeDatabase()
{
    if (m_db.isOpen()) {
        m_db.close();
    }
    // Remove connection
    if (QSqlDatabase::contains(m_dbConnectionName)) {
        QSqlDatabase::removeDatabase(m_dbConnectionName);
    }
}

void ChatWidget::saveMessageToDb(const QString &sender, const QString &text)
{
    if (!m_db.isOpen() || m_projectDir.isEmpty()) return;
    
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO chat_messages (sender, message) VALUES (:sender, :message)");
    query.bindValue(":sender", sender);
    query.bindValue(":message", text);
    
    if (!query.exec()) {
        qWarning() << "Failed to save chat message:" << query.lastError().text();
    }
}

void ChatWidget::saveChatHistory()
{
    // With SQLite, messages are saved immediately via saveMessageToDb
    // This method is kept for compatibility but doesn't need to do anything
}

void ChatWidget::loadChatHistory()
{
    if (!m_db.isOpen() || m_projectDir.isEmpty()) return;
    
    QSqlQuery query(m_db);
    query.exec("SELECT sender, message FROM chat_messages ORDER BY id ASC");
    
    while (query.next()) {
        QString sender = query.value(0).toString();
        QString text = query.value(1).toString();
        if (!sender.isEmpty() && !text.isEmpty()) {
            m_chatHistory.append(qMakePair(sender, text));
            // Add to view without re-adding to history/db
            appendMessage(sender, text, false);
        }
    }
}

void ChatWidget::clearChat()
{
    m_chatHistory.clear();
    conversationView->clear();
}