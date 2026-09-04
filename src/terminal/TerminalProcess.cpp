#include "terminal/TerminalProcess.h"

#include "core/AppSettings.h"
#include "core/Logging.h"
#include "terminal/ITerminalBackend.h"
#include "terminal/ProcessTerminalBackend.h"
#include "terminal/PtyTerminalBackend.h"
#include "terminal/ShellProfiles.h"

#include <QDir>
#include <QStringList>

TerminalProcess::TerminalProcess(QObject *parent)
    : QObject(parent)
    , m_workingDirectory(QDir::currentPath())
    , m_columns(80)
    , m_rows(24)
{
}

TerminalProcess::~TerminalProcess()
{
    stop();
}

void TerminalProcess::setWorkingDirectory(const QString &path)
{
    const QDir dir(path);
    if (dir.exists()) {
        m_workingDirectory = dir.absolutePath();
    }
}

void TerminalProcess::setSize(int columns, int rows)
{
    if (columns > 0) {
        m_columns = columns;
    }
    if (rows > 0) {
        m_rows = rows;
    }
    if (m_backend) {
        m_backend->resize(m_columns, m_rows);
    }
}

bool TerminalProcess::isRunning() const
{
    return m_backend && m_backend->isRunning();
}

bool TerminalProcess::isPty() const
{
    return m_backend && m_backend->isPty();
}

void TerminalProcess::bindBackend(ITerminalBackend *backend)
{
    m_backend = backend;
    connect(backend, &ITerminalBackend::dataReceived, this,
            [this](const QByteArray &data, bool isError) {
                emit outputReady(QString::fromLocal8Bit(data), isError);
            });
    connect(backend, &ITerminalBackend::finished, this, &TerminalProcess::finished);
    connect(backend, &ITerminalBackend::failed, this, &TerminalProcess::failed);
}

void TerminalProcess::startWithProcess(const QString &program, const QStringList &args)
{
    if (m_backend) {
        m_backend->deleteLater();
        m_backend = nullptr;
    }
    auto *backend = new ProcessTerminalBackend(this);
    bindBackend(backend);
    backend->start(program, args, m_workingDirectory, m_columns, m_rows);
}

void TerminalProcess::startCommand(const QString &command)
{
    if (isRunning() || command.trimmed().isEmpty()) {
        return;
    }

    QString shell = AppSettings().terminalShell();
    if (shell.isEmpty()) {
        shell = defaultShellPath();
    }
    const QStringList args = shellCommandArguments(shell, command);

    const bool wantPty = AppSettings().terminalUsePty() && PtyTerminalBackend::isAvailable();
    if (wantPty) {
        if (m_backend) {
            m_backend->deleteLater();
            m_backend = nullptr;
        }
        auto *pty = new PtyTerminalBackend(this);
        qCInfo(lcTerminal) << "Starting" << command << "via PTY" << shell << "in"
                           << m_workingDirectory;
        pty->start(shell, args, m_workingDirectory, m_columns, m_rows);
        if (pty->isRunning()) {
            bindBackend(pty);
            return;
        }
        pty->deleteLater();
    }

    qCInfo(lcTerminal) << "Starting" << command << "via" << shell << "in" << m_workingDirectory;
    startWithProcess(shell, args);
}

void TerminalProcess::stop()
{
    if (m_backend) {
        m_backend->stop();
    }
}
