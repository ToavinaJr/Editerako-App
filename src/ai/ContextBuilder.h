#ifndef EDITERAKO_CONTEXTBUILDER_H
#define EDITERAKO_CONTEXTBUILDER_H

#include "ai/ChatMessage.h"

#include <QList>
#include <QString>

class ContextBuilder
{
public:
    void setActiveFile(const QString &path, const QString &content);
    [[nodiscard]] QString buildPrompt(const QString &userMessage,
                                      const QList<ChatMessage> &history) const;

private:
    QString m_filePath;
    QString m_fileContent;
};

#endif
