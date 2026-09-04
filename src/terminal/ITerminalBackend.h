#ifndef EDITERAKO_ITERMINALBACKEND_H
#define EDITERAKO_ITERMINALBACKEND_H

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QStringList>

class ITerminalBackend : public QObject
{
    Q_OBJECT

public:
    explicit ITerminalBackend(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

    virtual void start(const QString &program, const QStringList &arguments,
                       const QString &workingDirectory, int columns, int rows) = 0;
    virtual void write(const QByteArray &data) = 0;
    virtual void resize(int columns, int rows) = 0;
    virtual void stop() = 0;
    [[nodiscard]] virtual bool isRunning() const = 0;
    [[nodiscard]] virtual bool isPty() const = 0;

signals:
    void dataReceived(const QByteArray &data, bool isError);
    void finished(int exitCode, bool crashed);
    void failed(const QString &message);
};

#endif
