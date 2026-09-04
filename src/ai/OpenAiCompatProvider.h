#ifndef EDITERAKO_OPENAICOMPATPROVIDER_H
#define EDITERAKO_OPENAICOMPATPROVIDER_H

#include "ai/AiProvider.h"

class QNetworkAccessManager;
class QNetworkReply;

class OpenAiCompatProvider final : public AiProvider
{
    Q_OBJECT

public:
    explicit OpenAiCompatProvider(const QString &providerId, QObject *parent = nullptr);
    ~OpenAiCompatProvider() override;

    void send(const QString &prompt) override;
    void cancel() override;
    [[nodiscard]] bool isBusy() const override;
    [[nodiscard]] QString displayName() const override;
    [[nodiscard]] QString providerId() const override { return m_id; }

private:
    void abortActiveReply();
    void onReplyFinished();

    QString m_id;
    QNetworkAccessManager *m_network = nullptr;
    QNetworkReply *m_reply = nullptr;
};

#endif
