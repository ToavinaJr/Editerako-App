#include "ai/AiResponseParse.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace {

QJsonObject objectFrom(const QByteArray &json)
{
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(json, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        return {};
    }
    return doc.object();
}

} // namespace

QString parseGeminiResponse(const QByteArray &json)
{
    const QJsonObject obj = objectFrom(json);
    const QJsonArray cand = obj.value(QStringLiteral("candidates")).toArray();
    if (cand.isEmpty() || !cand.at(0).isObject()) {
        return {};
    }
    const QJsonObject content = cand.at(0).toObject().value(QStringLiteral("content")).toObject();
    const QJsonArray parts = content.value(QStringLiteral("parts")).toArray();
    QString out;
    for (const QJsonValue &partVal : parts) {
        if (partVal.isObject()) {
            out += partVal.toObject().value(QStringLiteral("text")).toString();
        }
    }
    return out;
}

QString parseOpenAiChatResponse(const QByteArray &json)
{
    const QJsonObject obj = objectFrom(json);
    const QJsonArray choices = obj.value(QStringLiteral("choices")).toArray();
    if (choices.isEmpty() || !choices.at(0).isObject()) {
        return {};
    }
    const QJsonObject choice = choices.at(0).toObject();
    const QJsonObject message = choice.value(QStringLiteral("message")).toObject();
    QString text = message.value(QStringLiteral("content")).toString();
    if (text.isEmpty()) {
        text = choice.value(QStringLiteral("text")).toString();
    }
    return text;
}

QString parseAnthropicResponse(const QByteArray &json)
{
    const QJsonObject obj = objectFrom(json);
    const QJsonArray content = obj.value(QStringLiteral("content")).toArray();
    QString out;
    for (const QJsonValue &item : content) {
        if (!item.isObject()) {
            continue;
        }
        const QJsonObject block = item.toObject();
        if (block.value(QStringLiteral("type")).toString() == QLatin1String("text")) {
            out += block.value(QStringLiteral("text")).toString();
        }
    }
    return out;
}

QString parseOpenAiSseDelta(const QByteArray &line)
{
    QByteArray trimmed = line.trimmed();
    if (trimmed.startsWith("data:")) {
        trimmed = trimmed.mid(5).trimmed();
    }
    if (trimmed.isEmpty() || trimmed == "[DONE]") {
        return {};
    }
    const QJsonObject obj = objectFrom(trimmed);
    const QJsonArray choices = obj.value(QStringLiteral("choices")).toArray();
    if (choices.isEmpty()) {
        return {};
    }
    const QJsonObject delta = choices.at(0).toObject().value(QStringLiteral("delta")).toObject();
    return delta.value(QStringLiteral("content")).toString();
}
