#include "ai/GeminiProvider.h"

#include "ai/AiCatalog.h"
#include "ai/AiResponseParse.h"
#include "core/AppSettings.h"
#include "core/Logging.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

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
    return aiServiceById(QStringLiteral("gemini")).name;
}

bool GeminiProvider::isBusy() const
{
    return m_reply != nullptr;
}

void GeminiProvider::cancel()
{
    abortActiveReply();
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
        emit errorOccurred(tr("GEMINI_API_KEY is not set. Add it to .env, or pick ChatGPT / Claude "
                              "in the chat panel and sign in with your account."));
        return;
    }

    abortActiveReply();

    const AiService service = aiServiceById(QStringLiteral("gemini"));
    QString model = AppSettings().aiModel();
    if (model.isEmpty()) {
        model = service.defaultModel;
    }
    QString endpoint = AppSettings().aiEndpoint();
    if (endpoint.isEmpty()) {
        endpoint = service.defaultEndpoint.arg(model);
    }

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

    QUrl url(endpoint);
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

    const QString text = parseGeminiResponse(resp);
    emit responseReady(text.isEmpty() ? QString::fromUtf8(resp) : text);
}
