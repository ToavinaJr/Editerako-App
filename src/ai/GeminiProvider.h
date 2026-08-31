#ifndef EDITERAKO_GEMINIPROVIDER_H
#define EDITERAKO_GEMINIPROVIDER_H

#include "ai/AiProvider.h"

class QNetworkAccessManager;
class QNetworkReply;

class GeminiProvider : public AiProvider
{
    Q_OBJECT

public:
    explicit GeminiProvider(QObject *parent = nullptr);
    ~GeminiProvider() override;

    void send(const QString &prompt) override;
    [[nodiscard]] QString displayName() const override;

private:
    void abortActiveReply();
    void onReplyFinished();

    QNetworkAccessManager *m_network = nullptr;
    QNetworkReply *m_reply = nullptr;
};

#endif
