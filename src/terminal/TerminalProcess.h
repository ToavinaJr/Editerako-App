#ifndef EDITERAKO_TERMINALPROCESS_H
#define EDITERAKO_TERMINALPROCESS_H

#include <QObject>
#include <QString>

class QProcess;

class TerminalProcess : public QObject
{
    Q_OBJECT

public:
    explicit TerminalProcess(QObject *parent = nullptr);
    ~TerminalProcess() override;

    void setWorkingDirectory(const QString &path);
    [[nodiscard]] QString workingDirectory() const { return m_workingDirectory; }
    [[nodiscard]] bool isRunning() const;

    void startCommand(const QString &command);
    void stop();

signals:
    void outputReady(const QString &text, bool isError);
    void finished(int exitCode, bool crashed);
    void failed(const QString &message);

private:
    void onReadyRead();
    [[nodiscard]] static QString defaultShell();

    QProcess *m_process = nullptr;
    QString m_workingDirectory;
    QString m_shell;
    bool m_stopping = false;
};

#endif
