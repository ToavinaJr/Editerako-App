#ifndef EDITERAKO_PROCESSTERMINALBACKEND_H
#define EDITERAKO_PROCESSTERMINALBACKEND_H

#include "terminal/ITerminalBackend.h"

class QProcess;

class ProcessTerminalBackend final : public ITerminalBackend
{
    Q_OBJECT

public:
    explicit ProcessTerminalBackend(QObject *parent = nullptr);
    ~ProcessTerminalBackend() override;

    void start(const QString &program, const QStringList &arguments,
               const QString &workingDirectory, int columns, int rows) override;
    void write(const QByteArray &data) override;
    void resize(int columns, int rows) override;
    void stop() override;
    [[nodiscard]] bool isRunning() const override;
    [[nodiscard]] bool isPty() const override { return false; }

private:
    void onReadyRead();

    QProcess *m_process = nullptr;
    int m_columns = 80;
    int m_rows = 24;
    bool m_stopping = false;
};

#endif
