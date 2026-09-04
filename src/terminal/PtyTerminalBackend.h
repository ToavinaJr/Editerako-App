#ifndef EDITERAKO_PTYTERMINALBACKEND_H
#define EDITERAKO_PTYTERMINALBACKEND_H

#include "terminal/ITerminalBackend.h"

class PtyTerminalBackend final : public ITerminalBackend
{
    Q_OBJECT

public:
    explicit PtyTerminalBackend(QObject *parent = nullptr);
    ~PtyTerminalBackend() override;

    [[nodiscard]] static bool isAvailable();

    void start(const QString &program, const QStringList &arguments,
               const QString &workingDirectory, int columns, int rows) override;
    void write(const QByteArray &data) override;
    void resize(int columns, int rows) override;
    void stop() override;
    [[nodiscard]] bool isRunning() const override;
    [[nodiscard]] bool isPty() const override { return true; }

private:
    struct Impl;
    Impl *m_impl = nullptr;
};

#endif
