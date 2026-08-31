#include "ai/GeminiProvider.h"

#include "ai/AiProvider.h"
#include "core/AppSettings.h"
#include "core/Logging.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

AiProvider *AiProvider::create(QObject *parent)
{
    const QString id = AppSettings().aiProvider();
    if (!id.isEmpty() && id != QLatin1String("gemini")) {
        qCWarning(lcAi) << "Unknown AI provider" << id << "- using Gemini";
    }
    return new GeminiProvider(parent);
}

GeminiProvider::GeminiProvider(QObject *parent)
    : AiProvider(parent)
    , m_network(new QNetworkAccessManager(this))
{
}

GeminiProvider::~GeminiProvider()
{
    abortActiveReply();
}

QString GeminiProvider::displayName() const
{
    return QStringLiteral("Gemini");
}

void GeminiProvider::abortActiveReply()
{
    if (!m_reply) {
        return;
    }
    QNetworkReply *reply = m_reply;
    m_reply = nullptr;
    reply->disconnect(this);
    reply->abort();
    reply->deleteLater();
}

void GeminiProvider::send(const QString &prompt)
{
    const QByteArray apiKey = qgetenv("GEMINI_API_KEY");
    if (apiKey.isEmpty()) {
        emit errorOccurred(tr("GEMINI_API_KEY not set in environment. Set it and retry."));
        return;
    }

    abortActiveReply();

    QJsonObject partObj;
    partObj.insert(QStringLiteral("text"), prompt);
    QJsonArray partsArr;
    partsArr.append(partObj);

    QJsonObject contentObj;
    contentObj.insert(QStringLiteral("parts"), partsArr);
    QJsonArray contentsArr;
    contentsArr.append(contentObj);

    QJsonObject root;
    root.insert(QStringLiteral("contents"), contentsArr);

    const QByteArray body = QJsonDocument(root).toJson(QJsonDocument::Compact);

    QUrl url(QStringLiteral(
        "https://generativelanguage.googleapis.com/v1beta/models/gemini-2.0-flash-001:generateContent"));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader("x-goog-api-key", apiKey);

    m_reply = m_network->post(request, body);
    connect(m_reply, &QNetworkReply::finished, this, &GeminiProvider::onReplyFinished);
}

void GeminiProvider::onReplyFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply) {
        return;
    }
    if (m_reply == reply) {
        m_reply = nullptr;
    }

    const QByteArray resp = reply->readAll();
    const QNetworkReply::NetworkError error = reply->error();
    const QString errorString = reply->errorString();
    reply->deleteLater();

    if (error == QNetworkReply::OperationCanceledError) {
        return;
    }

    if (error != QNetworkReply::NoError) {
        QString errMsg = errorString;
        if (!resp.isEmpty()) {
            errMsg += QStringLiteral("\nServer response: %1").arg(QString::fromUtf8(resp));
        }
        qCWarning(lcAi) << "Gemini request failed:" << errMsg;
        emit errorOccurred(errMsg);
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument jdoc = QJsonDocument::fromJson(resp, &parseError);
    QString outText;
    if (parseError.error == QJsonParseError::NoError && jdoc.isObject()) {
        const QJsonObject obj = jdoc.object();
        const QJsonArray cand = obj.value(QStringLiteral("candidates")).toArray();
        if (!cand.isEmpty() && cand.at(0).isObject()) {
            const QJsonObject content = cand.at(0).toObject().value(QStringLiteral("content")).toObject();
            const QJsonArray parts = content.value(QStringLiteral("parts")).toArray();
            for (const QJsonValue &partVal : parts) {
                if (partVal.isObject()) {
                    outText += partVal.toObject().value(QStringLiteral("text")).toString();
                }
            }
        }
        if (outText.isEmpty()) {
            outText = QString::fromUtf8(resp);
        }
    } else {
        outText = QString::fromUtf8(resp);
    }

    emit responseReady(outText);
}
