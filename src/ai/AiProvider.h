#ifndef EDITERAKO_AIPROVIDER_H
#define EDITERAKO_AIPROVIDER_H

#include <QObject>
#include <QString>

class AiProvider : public QObject
{
    Q_OBJECT

public:
    explicit AiProvider(QObject *parent = nullptr);
    ~AiProvider() override = default;

    [[nodiscard]] static AiProvider *create(QObject *parent = nullptr);
    [[nodiscard]] static AiProvider *create(const QString &providerId, QObject *parent = nullptr);

    virtual void send(const QString &prompt) = 0;
    virtual void cancel();
    [[nodiscard]] virtual bool isBusy() const;
    [[nodiscard]] virtual QString displayName() const = 0;
    [[nodiscard]] virtual QString providerId() const = 0;

signals:
    void responseReady(const QString &text);
    void errorOccurred(const QString &message);
};

#endif
