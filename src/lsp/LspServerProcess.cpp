#include "lsp/LspServerProcess.h"

#include "core/Logging.h"
#include "lsp/JsonRpcTransport.h"

#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>

namespace {

bool lspCommandExists(const QString &command)
{
    const QFileInfo info(command);
    if (info.isAbsolute() || command.contains(u'/') || command.contains(u'\\')) {
        return info.isFile();
    }
    return !QStandardPaths::findExecutable(command).isEmpty();
}

} // namespace

LspServerProcess::LspServerProcess(QObject *parent)
    : QObject(parent)
{
}

LspServerProcess::~LspServerProcess()
{
    stop();
}

bool LspServerProcess::start(const QString &command, const QStringList &args,
                             const QString &workingDirectory)
{
    if (command.trimmed().isEmpty()) {
        const QString message = QStringLiteral("LSP server command is empty");
        qCWarning(lcLsp) << message;
        emit failed(message);
        return false;
    }
    if (!lspCommandExists(command)) {
        const QString message = QStringLiteral("LSP server binary not found: %1").arg(command);
        qCWarning(lcLsp) << message;
        emit failed(message);
        return false;
    }

    stop();
    m_process = new QProcess(this);
    m_process->setProcessChannelMode(QProcess::SeparateChannels);
    if (!workingDirectory.isEmpty()) {
        m_process->setWorkingDirectory(workingDirectory);
    }
    m_transport = new ProcessJsonRpcTransport(m_process, this);

    connect(m_process, &QProcess::started, this, &LspServerProcess::started);
    connect(m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError) {
        const QString message = m_process ? m_process->errorString()
                                          : QStringLiteral("LSP process error");
        qCWarning(lcLsp) << message;
        emit failed(message);
    });
    connect(m_process, &QProcess::readyReadStandardError, this, [this]() {
        const QByteArray err = m_process->readAllStandardError();
        if (!err.isEmpty()) {
            qCWarning(lcLsp).noquote() << QString::fromUtf8(err.trimmed());
        }
    });
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int, QProcess::ExitStatus) { emit finished(); });

    m_process->start(command, args);
    return true;
}

void LspServerProcess::stop()
{
    if (m_transport) {
        delete m_transport;
        m_transport = nullptr;
    }
    if (!m_process) {
        return;
    }
    if (m_process->state() != QProcess::NotRunning) {
        m_process->terminate();
        if (!m_process->waitForFinished(1500)) {
            m_process->kill();
            m_process->waitForFinished(500);
        }
    }
    m_process->deleteLater();
    m_process = nullptr;
}

bool LspServerProcess::isRunning() const
{
    return m_process && m_process->state() == QProcess::Running;
}

JsonRpcTransport *LspServerProcess::transport() const
{
    return m_transport;
}
