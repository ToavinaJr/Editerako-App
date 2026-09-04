#ifndef EDITERAKO_TERMINALPROCESS_H
#define EDITERAKO_TERMINALPROCESS_H

#include <QObject>
#include <QString>
#include <QStringList>

class ITerminalBackend;

class TerminalProcess : public QObject
{
    Q_OBJECT

public:
    explicit TerminalProcess(QObject *parent = nullptr);
    ~TerminalProcess() override;

    void setWorkingDirectory(const QString &path);
    [[nodiscard]] QString workingDirectory() const { return m_workingDirectory; }
    void setSize(int columns, int rows);
    [[nodiscard]] bool isRunning() const;
    [[nodiscard]] bool isPty() const;

    void startCommand(const QString &command);
    void stop();

signals:
    void outputReady(const QString &text, bool isError);
    void finished(int exitCode, bool crashed);
    void failed(const QString &message);

private:
    void bindBackend(ITerminalBackend *backend);
    void startWithProcess(const QString &program, const QStringList &args);

    ITerminalBackend *m_backend = nullptr;
    QString m_workingDirectory;
    int m_columns = 80;
    int m_rows = 24;
};

#endif
