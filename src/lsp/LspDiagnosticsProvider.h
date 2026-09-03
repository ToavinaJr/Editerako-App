#ifndef EDITERAKO_LSPDIAGNOSTICSPROVIDER_H
#define EDITERAKO_LSPDIAGNOSTICSPROVIDER_H

#include "lsp/LspTypes.h"

#include <QJsonValue>
#include <QObject>
#include <QString>
#include <QVector>

class LspClient;

class LspDiagnosticsProvider : public QObject
{
    Q_OBJECT

public:
    explicit LspDiagnosticsProvider(LspClient *client, QObject *parent = nullptr);

signals:
    void diagnosticsPublished(const QString &uri, const QVector<LspDiagnostic> &diagnostics);

private:
    void onNotification(const QString &method, const QJsonValue &params);
};

#endif
