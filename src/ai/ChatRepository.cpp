#include "ai/ChatRepository.h"

#include "core/Logging.h"

#include <QDir>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>

ChatRepository::ChatRepository()
    : m_connectionName(QUuid::createUuid().toString())
{
}

ChatRepository::~ChatRepository()
{
    close();
}

QString ChatRepository::databaseFilePath() const
{
    if (m_projectDir.isEmpty()) {
        return {};
    }
    return QDir(m_projectDir).filePath(QStringLiteral(".editerako/chat_history.db"));
}

bool ChatRepository::ensureOpen() const
{
    if (!QSqlDatabase::contains(m_connectionName)) {
        return false;
    }
    return QSqlDatabase::database(m_connectionName).isOpen();
}

bool ChatRepository::isOpen() const
{
    return ensureOpen();
}

void ChatRepository::close()
{
    if (QSqlDatabase::contains(m_connectionName)) {
        {
            QSqlDatabase db = QSqlDatabase::database(m_connectionName);
            if (db.isOpen()) {
                db.close();
            }
        }
        QSqlDatabase::removeDatabase(m_connectionName);
    }
}

bool ChatRepository::open(const QString &projectDir)
{
    close();
    m_projectDir = projectDir;
    if (m_projectDir.isEmpty()) {
        return false;
    }

    QDir dir(m_projectDir);
    if (!dir.exists(QStringLiteral(".editerako")) && !dir.mkdir(QStringLiteral(".editerako"))) {
        qCWarning(lcAi) << "Could not create .editerako directory in" << m_projectDir;
        return false;
    }

    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    db.setDatabaseName(databaseFilePath());
    if (!db.open()) {
        qCWarning(lcAi) << "Failed to open chat history database:" << db.lastError().text();
        close();
        return false;
    }

    QSqlQuery query(db);
    if (!query.exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS chat_messages ("
            "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "  sender TEXT NOT NULL,"
            "  message TEXT NOT NULL,"
            "  timestamp DATETIME DEFAULT CURRENT_TIMESTAMP"
            ")"))) {
        qCWarning(lcAi) << "Failed to create chat_messages table:" << query.lastError().text();
        close();
        return false;
    }
    return true;
}

bool ChatRepository::append(const ChatMessage &message)
{
    if (!ensureOpen() || m_projectDir.isEmpty()) {
        return false;
    }

    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare(QStringLiteral(
        "INSERT INTO chat_messages (sender, message) VALUES (:sender, :message)"));
    query.bindValue(QStringLiteral(":sender"), message.sender);
    query.bindValue(QStringLiteral(":message"), message.text);
    if (!query.exec()) {
        qCWarning(lcAi) << "Failed to save chat message:" << query.lastError().text();
        return false;
    }
    return true;
}

QList<ChatMessage> ChatRepository::loadAll() const
{
    QList<ChatMessage> result;
    if (!ensureOpen() || m_projectDir.isEmpty()) {
        return result;
    }

    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    if (!query.exec(QStringLiteral("SELECT sender, message FROM chat_messages ORDER BY id ASC"))) {
        qCWarning(lcAi) << "Failed to load chat history:" << query.lastError().text();
        return result;
    }

    while (query.next()) {
        ChatMessage message;
        message.sender = query.value(0).toString();
        message.text = query.value(1).toString();
        if (!message.sender.isEmpty() && !message.text.isEmpty()) {
            result.append(message);
        }
    }
    return result;
}
