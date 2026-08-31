#include "ai/ContextBuilder.h"

#include <QStringList>
#include <QtGlobal>

namespace {

constexpr int kMaxFileChars = 8000;
constexpr int kMaxHistoryMessages = 8;

QString truncated(const QString &text)
{
    if (text.size() <= kMaxFileChars) {
        return text;
    }
    return text.left(kMaxFileChars) + QStringLiteral("\n... [truncated]");
}

} // namespace

void ContextBuilder::setActiveFile(const QString &path, const QString &content)
{
    m_filePath = path;
    m_fileContent = content;
}

QString ContextBuilder::buildPrompt(const QString &userMessage,
                                    const QList<ChatMessage> &history) const
{
    QStringList parts;

    if (!m_filePath.isEmpty()) {
        parts << QStringLiteral("Current file: %1").arg(m_filePath);
        if (!m_fileContent.isEmpty()) {
            parts << QStringLiteral("```\n%1\n```").arg(truncated(m_fileContent));
        }
    }

    const int start = qMax(0, static_cast<int>(history.size()) - kMaxHistoryMessages);
    for (int i = start; i < history.size(); ++i) {
        const ChatMessage &msg = history.at(i);
        if (i == history.size() - 1 && msg.text == userMessage) {
            continue;
        }
        parts << QStringLiteral("%1: %2").arg(msg.sender, msg.text);
    }

    parts << userMessage;
    return parts.join(QStringLiteral("\n\n"));
}
