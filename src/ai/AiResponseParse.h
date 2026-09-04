#ifndef EDITERAKO_AIRESPONSEPARSE_H
#define EDITERAKO_AIRESPONSEPARSE_H

#include <QString>

[[nodiscard]] QString parseGeminiResponse(const QByteArray &json);
[[nodiscard]] QString parseOpenAiChatResponse(const QByteArray &json);
[[nodiscard]] QString parseAnthropicResponse(const QByteArray &json);
[[nodiscard]] QString parseOpenAiSseDelta(const QByteArray &line);

#endif
