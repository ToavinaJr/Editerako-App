#ifndef EDITERAKO_AICATALOG_H
#define EDITERAKO_AICATALOG_H

#include <QString>
#include <QUrl>
#include <QVector>

struct AiService {
    QString id;
    QString name;
    enum class Kind { Account, Api };
    Kind kind = Kind::Account;
    QUrl accountUrl;
    QString defaultModel;
    QString defaultEndpoint;
    QString apiKeyEnv;
};

[[nodiscard]] QVector<AiService> aiServices();
[[nodiscard]] AiService aiServiceById(const QString &id);
[[nodiscard]] QString defaultAiProviderId();
[[nodiscard]] bool isAccountAiProvider(const QString &id);

#endif
