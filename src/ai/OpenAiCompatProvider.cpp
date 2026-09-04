#include "ai/OpenAiCompatProvider.h"

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

OpenAiCompatProvider::OpenAiCompatProvider(const QString &providerId, QObject *parent)
    : AiProvider(parent)
    , m_id(providerId)
    , m_network(new QNetworkAccessManager(this))
{
}

OpenAiCompatProvider::~OpenAiCompatProvider()
{
    abortActiveReply();
}

QString OpenAiCompatProvider::displayName() const
{
    return aiServiceById(m_id).name;
}

bool OpenAiCompatProvider::isBusy() const
{
    return m_reply != nullptr;
}

void OpenAiCompatProvider::cancel()
{
    abortActiveReply();
}

void OpenAiCompatProvider::abortActiveReply()
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

void OpenAiCompatProvider::send(const QString &prompt)
{
    const AiService service = aiServiceById(m_id);
    QByteArray apiKey;
    if (!service.apiKeyEnv.isEmpty()) {
        apiKey = qgetenv(service.apiKeyEnv.toUtf8().constData());
        if (apiKey.isEmpty()) {
            emit errorOccurred(
                tr("%1 is not set. Add it to .env or the environment, or use an account chat (Sign in).")
                    .arg(service.apiKeyEnv));
            return;
        }
    }

    abortActiveReply();

    QString model = AppSettings().aiModel();
    if (model.isEmpty() || (m_id != QLatin1String("gemini") && model.contains(QLatin1String("gemini")))) {
        model = service.defaultModel;
    }
    QString endpoint = AppSettings().aiEndpoint();
    if (endpoint.isEmpty()) {
        endpoint = service.defaultEndpoint;
    }

    QJsonObject message{{QStringLiteral("role"), QStringLiteral("user")},
                        {QStringLiteral("content"), prompt}};
    QJsonObject root{{QStringLiteral("model"), model},
                     {QStringLiteral("messages"), QJsonArray{message}}};

    QNetworkRequest request{QUrl(endpoint)};
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    if (!apiKey.isEmpty()) {
        request.setRawHeader("Authorization", QByteArray("Bearer ") + apiKey);
    }

    m_reply = m_network->post(request, QJsonDocument(root).toJson(QJsonDocument::Compact));
    connect(m_reply, &QNetworkReply::finished, this, &OpenAiCompatProvider::onReplyFinished);
}

void OpenAiCompatProvider::onReplyFinished()
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
            errMsg += QStringLiteral("\n%1").arg(QString::fromUtf8(resp.left(800)));
        }
        qCWarning(lcAi) << m_id << "request failed:" << errMsg;
        emit errorOccurred(errMsg);
        return;
    }

    const QString text = parseOpenAiChatResponse(resp);
    emit responseReady(text.isEmpty() ? QString::fromUtf8(resp) : text);
}
