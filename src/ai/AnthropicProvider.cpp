#include "ai/AnthropicProvider.h"

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

AnthropicProvider::AnthropicProvider(QObject *parent)
    : AiProvider(parent)
    , m_network(new QNetworkAccessManager(this))
{
}

AnthropicProvider::~AnthropicProvider()
{
    abortActiveReply();
}

QString AnthropicProvider::displayName() const
{
    return aiServiceById(QStringLiteral("anthropic")).name;
}

bool AnthropicProvider::isBusy() const
{
    return m_reply != nullptr;
}

void AnthropicProvider::cancel()
{
    abortActiveReply();
}

void AnthropicProvider::abortActiveReply()
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

void AnthropicProvider::send(const QString &prompt)
{
    const AiService service = aiServiceById(QStringLiteral("anthropic"));
    const QByteArray apiKey = qgetenv(service.apiKeyEnv.toUtf8().constData());
    if (apiKey.isEmpty()) {
        emit errorOccurred(
            tr("ANTHROPIC_API_KEY is not set. Add it to .env, or use an account chat (Sign in)."));
        return;
    }

    abortActiveReply();

    QString model = AppSettings().aiModel();
    if (model.isEmpty() || model.contains(QLatin1String("gemini"))
        || model.startsWith(QLatin1String("gpt-"))) {
        model = service.defaultModel;
    }
    QString endpoint = AppSettings().aiEndpoint();
    if (endpoint.isEmpty()) {
        endpoint = service.defaultEndpoint;
    }

    QJsonObject message{{QStringLiteral("role"), QStringLiteral("user")},
                        {QStringLiteral("content"), prompt}};
    QJsonObject root{{QStringLiteral("model"), model},
                     {QStringLiteral("max_tokens"), 4096},
                     {QStringLiteral("messages"), QJsonArray{message}}};

    QNetworkRequest request{QUrl(endpoint)};
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader("x-api-key", apiKey);
    request.setRawHeader("anthropic-version", "2023-06-01");

    m_reply = m_network->post(request, QJsonDocument(root).toJson(QJsonDocument::Compact));
    connect(m_reply, &QNetworkReply::finished, this, &AnthropicProvider::onReplyFinished);
}

void AnthropicProvider::onReplyFinished()
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
        qCWarning(lcAi) << "Anthropic request failed:" << errMsg;
        emit errorOccurred(errMsg);
        return;
    }

    const QString text = parseAnthropicResponse(resp);
    emit responseReady(text.isEmpty() ? QString::fromUtf8(resp) : text);
}
