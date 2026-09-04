#ifndef EDITERAKO_ANTHROPICPROVIDER_H
#define EDITERAKO_ANTHROPICPROVIDER_H

#include "ai/AiProvider.h"

class QNetworkAccessManager;
class QNetworkReply;

class AnthropicProvider final : public AiProvider
{
    Q_OBJECT

public:
    explicit AnthropicProvider(QObject *parent = nullptr);
    ~AnthropicProvider() override;

    void send(const QString &prompt) override;
    void cancel() override;
    [[nodiscard]] bool isBusy() const override;
    [[nodiscard]] QString displayName() const override;
    [[nodiscard]] QString providerId() const override { return QStringLiteral("anthropic"); }

private:
    void abortActiveReply();
    void onReplyFinished();

    QNetworkAccessManager *m_network = nullptr;
    QNetworkReply *m_reply = nullptr;
};

#endif
