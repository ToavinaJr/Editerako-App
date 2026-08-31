#ifndef EDITERAKO_CHATREPOSITORY_H
#define EDITERAKO_CHATREPOSITORY_H

#include "ai/ChatMessage.h"

#include <QList>
#include <QString>

class ChatRepository
{
public:
    ChatRepository();
    ~ChatRepository();

    ChatRepository(const ChatRepository &) = delete;
    ChatRepository &operator=(const ChatRepository &) = delete;

    bool open(const QString &projectDir);
    void close();
    [[nodiscard]] bool isOpen() const;

    bool append(const ChatMessage &message);
    [[nodiscard]] QList<ChatMessage> loadAll() const;

private:
    [[nodiscard]] QString databaseFilePath() const;
    bool ensureOpen() const;

    QString m_projectDir;
    QString m_connectionName;
};

#endif
