#include "app/DebugSession.h"

#include "core/Logging.h"
#include "debug/DapClient.h"
#include "editor/CodeEditor.h"
#include "editor/EditorDocument.h"
#include "editor/EditorManager.h"
#include "lsp/LspServerProcess.h"

#include <QTimer>

DebugSession::DebugSession(EditorManager *editors, QObject *parent)
    : QObject(parent)
    , m_editors(editors)
    , m_handshakeTimer(new QTimer(this))
{
    m_handshakeTimer->setSingleShot(true);
    m_handshakeTimer->setInterval(15000);
    connect(m_handshakeTimer, &QTimer::timeout, this, [this]() {
        failStart(tr("Debug adapter did not initialize. gdb 14+ with --interpreter=dap "
                     "(or lldb-dap) is required."));
    });

    if (m_editors) {
        connect(m_editors, &EditorManager::documentOpened, this, &DebugSession::onDocumentOpened);
        for (CodeEditor *editor : m_editors->editors()) {
            attachEditor(editor);
            applyEditorDecorations(editor);
        }
    }
}

DebugSession::~DebugSession()
{
    m_stopping = true;
    destroyAdapter();
}

void DebugSession::setWorkspaceRoot(const QString &root)
{
    m_workspaceRoot = root;
    reloadConfigurations();
}

void DebugSession::setSelectedConfiguration(const QString &name)
{
    m_selectedName = name;
}

void DebugSession::reloadConfigurations()
{
    QString error;
    m_configurations = loadLaunchFile(m_workspaceRoot, &error);
    if (!error.isEmpty()) {
        qCWarning(lcDap) << error;
        appendConsole(tr("launch.json: %1").arg(error));
    }
    if (!m_selectedName.isEmpty()) {
        bool found = false;
        for (const LaunchConfiguration &config : m_configurations) {
            if (config.name == m_selectedName) {
                found = true;
                break;
            }
        }
        if (!found) {
            m_selectedName.clear();
        }
    }
    emit configurationsChanged();
}

bool DebugSession::createLaunchFile(QString *error)
{
    if (!writeLaunchTemplate(m_workspaceRoot, error)) {
        return false;
    }
    reloadConfigurations();
    return true;
}

LaunchContext DebugSession::currentContext() const
{
    return LaunchContext{m_workspaceRoot, currentFilePath()};
}

QString DebugSession::currentFilePath() const
{
    return m_editors ? m_editors->currentFilePath() : QString();
}

LaunchConfiguration DebugSession::selectedOrFirst() const
{
    for (const LaunchConfiguration &config : m_configurations) {
        if (!m_selectedName.isEmpty() && config.name == m_selectedName) {
            return config;
        }
    }
    if (!m_configurations.isEmpty()) {
        return m_configurations.front();
    }
    return {};
}

bool DebugSession::adapterActive() const
{
    return m_client != nullptr && m_process != nullptr;
}

void DebugSession::setState(State state)
{
    if (m_state == state) {
        return;
    }
    m_state = state;
    QString text;
    switch (state) {
    case State::Starting:
        text = tr("Debug: starting");
        break;
    case State::Running:
        text = tr("Debug: running");
        break;
    case State::Stopped:
        text = tr("Debug: paused");
        break;
    case State::Terminated:
        text = tr("Debug: ended");
        break;
    case State::Idle:
        break;
    }
    emit debugStatusChanged(text);
    emit stateChanged(state);
}

void DebugSession::appendConsole(const QString &text)
{
    emit outputReceived(QStringLiteral("console"), text.endsWith(QLatin1Char('\n'))
                                                       ? text
                                                       : text + QLatin1Char('\n'));
}

void DebugSession::onDocumentOpened(CodeEditor *editor)
{
    attachEditor(editor);
    applyEditorDecorations(editor);
}

void DebugSession::attachEditor(CodeEditor *editor)
{
    if (!editor || editor->property("dapAttached").toBool()) {
        return;
    }
    editor->setProperty("dapAttached", true);
    connect(editor, &CodeEditor::breakpointToggled, this, [this, editor](int line0) {
        auto *doc = EditorDocument::fromEditor(editor);
        if (!doc || doc->filePath().isEmpty()) {
            emit statusMessage(tr("Save the file before setting a breakpoint"), 2500);
            return;
        }
        toggleBreakpoint(doc->filePath(), line0);
    });
}

void DebugSession::applyEditorDecorations()
{
    if (!m_editors) {
        return;
    }
    for (CodeEditor *editor : m_editors->editors()) {
        applyEditorDecorations(editor);
    }
}

void DebugSession::applyEditorDecorations(CodeEditor *editor)
{
    if (!editor) {
        return;
    }
    auto *doc = EditorDocument::fromEditor(editor);
    const QString path = doc ? doc->filePath() : QString();
    editor->setBreakpointLines(m_breakpoints.linesFor(path));
    if (!path.isEmpty() && path == m_stoppedPath) {
        editor->setDebugLine(m_stoppedLine0);
    } else {
        editor->setDebugLine(-1);
    }
}

void DebugSession::clearDebugLine()
{
    m_stoppedPath.clear();
    m_stoppedLine0 = -1;
    applyEditorDecorations();
}

void DebugSession::toggleBreakpointAtCursor()
{
    if (!m_editors) {
        return;
    }
    CodeEditor *editor = m_editors->currentEditor();
    auto *doc = EditorDocument::fromEditor(editor);
    if (!editor || !doc || doc->filePath().isEmpty()) {
        emit statusMessage(tr("Save the file before setting a breakpoint"), 2500);
        return;
    }
    toggleBreakpoint(doc->filePath(), editor->textCursor().blockNumber());
}

void DebugSession::toggleBreakpoint(const QString &path, int line0)
{
    m_breakpoints.toggle(path, line0);
    applyEditorDecorations();
    if (!adapterActive() || (m_state != State::Running && m_state != State::Stopped
                             && m_state != State::Starting)) {
        return;
    }
    m_client->setBreakpoints(BreakpointStore::normalizePath(path),
                             m_breakpoints.sortedLinesFor(path), {});
}

void DebugSession::destroyAdapter()
{
    if (m_handshakeTimer) {
        m_handshakeTimer->stop();
    }
    m_pendingBreakpointSets = 0;
    m_frameId = 0;
    m_frames.clear();
    if (m_client) {
        disconnect(m_client, nullptr, this, nullptr);
        m_client->deleteLater();
        m_client = nullptr;
    }
    if (m_process) {
        disconnect(m_process, nullptr, this, nullptr);
        m_process->stop();
        m_process->deleteLater();
        m_process = nullptr;
    }
    clearDebugLine();
}

void DebugSession::failStart(const QString &message)
{
    qCWarning(lcDap) << message;
    appendConsole(message);
    emit statusMessage(message, 5000);
    m_handshakeTimer->stop();
    destroyAdapter();
    setState(State::Idle);
}

void DebugSession::start()
{
    emit panelRevealRequested();
    if (m_workspaceRoot.isEmpty()) {
        emit statusMessage(tr("Open a folder to debug"), 3000);
        appendConsole(tr("Open a folder to debug."));
        return;
    }

    if (m_state == State::Stopped) {
        continueRun();
        return;
    }
    if (m_state == State::Running || m_state == State::Starting) {
        return;
    }

    reloadConfigurations();
    if (m_configurations.isEmpty()) {
        const QString message =
            tr("No launch.json. Use Debug → Create launch.json to add a template.");
        appendConsole(message);
        emit statusMessage(message, 5000);
        return;
    }

    LaunchConfiguration config = expandLaunchConfiguration(selectedOrFirst(), currentContext());
    QString adapterError;
    if (!resolveDebugAdapter(&config, &adapterError)) {
        failStart(adapterError);
        return;
    }

    m_stopping = false;
    destroyAdapter();
    setState(State::Starting);
    appendConsole(tr("Starting %1 (%2 %3)...")
                      .arg(config.name, config.adapterCommand, config.adapterArgs.join(QLatin1Char(' '))));

    m_process = new LspServerProcess(this);
    connect(m_process, &LspServerProcess::failed, this, [this](const QString &message) {
        failStart(tr("Debug adapter failed: %1").arg(message));
    });
    connect(m_process, &LspServerProcess::finished, this, [this]() {
        if (m_stopping || m_state == State::Idle) {
            return;
        }
        appendConsole(tr("Debug adapter exited."));
        destroyAdapter();
        setState(State::Idle);
    });
    connect(m_process, &LspServerProcess::started, this, [this, config]() { beginHandshake(config); });

    const QString cwd = config.arguments.value(QStringLiteral("cwd")).toString();
    if (!m_process->start(config.adapterCommand, config.adapterArgs,
                          cwd.isEmpty() ? m_workspaceRoot : cwd)) {
        return;
    }
    m_handshakeTimer->start();
}

void DebugSession::beginHandshake(const LaunchConfiguration &config)
{
    if (!m_process || !m_process->transport()) {
        failStart(tr("Debug adapter started without a transport"));
        return;
    }
    m_client = new DapClient(m_process->transport(), this);
    connect(m_client, &DapClient::protocolError, this, [this](const QString &message) {
        appendConsole(tr("DAP: %1").arg(message));
    });
    connect(m_client, &DapClient::initializedEvent, this, &DebugSession::onInitializedEvent);
    connect(m_client, &DapClient::stopped, this, &DebugSession::onStopped);
    connect(m_client, &DapClient::continued, this, &DebugSession::onContinued);
    connect(m_client, &DapClient::terminated, this, &DebugSession::onTerminated);
    connect(m_client, &DapClient::exited, this, [this](int exitCode) {
        appendConsole(tr("Debuggee exited with code %1").arg(exitCode));
    });
    connect(m_client, &DapClient::output, this, &DebugSession::outputReceived);

    m_client->initialize(config.type, [this, config](const QJsonObject &, const QJsonObject &error) {
        if (!error.isEmpty()) {
            failStart(tr("initialize failed: %1").arg(error.value(QStringLiteral("message")).toString()));
            return;
        }
        sendLaunchOrAttach(config);
    });
}

void DebugSession::sendLaunchOrAttach(const LaunchConfiguration &config)
{
    if (!m_client) {
        return;
    }
    const auto onLaunch = [this](const QJsonObject &, const QJsonObject &error) {
        if (!error.isEmpty()) {
            failStart(tr("launch/attach failed: %1")
                          .arg(error.value(QStringLiteral("message")).toString()));
        }
    };
    if (config.request == QLatin1String("attach")) {
        m_client->attach(config.arguments, onLaunch);
    } else {
        m_client->launch(config.arguments, onLaunch);
    }
}

void DebugSession::onInitializedEvent()
{
    syncBreakpointsThenConfigDone();
}

void DebugSession::syncBreakpointsThenConfigDone()
{
    if (!m_client) {
        return;
    }

    const QHash<QString, QSet<int>> all = m_breakpoints.all();
    if (all.isEmpty()) {
        if (m_client->supportsConfigurationDone()) {
            m_client->configurationDone({});
        }
        m_handshakeTimer->stop();
        if (m_state == State::Starting) {
            setState(State::Running);
        }
        return;
    }

    m_pendingBreakpointSets = all.size();
    for (auto it = all.cbegin(); it != all.cend(); ++it) {
        m_client->setBreakpoints(it.key(), m_breakpoints.sortedLinesFor(it.key()),
                                 [this](const QJsonObject &, const QJsonObject &error) {
                                     if (!error.isEmpty()) {
                                         appendConsole(tr("setBreakpoints: %1")
                                                           .arg(error.value(QStringLiteral("message")).toString()));
                                     }
                                     --m_pendingBreakpointSets;
                                     if (m_pendingBreakpointSets > 0 || !m_client) {
                                         return;
                                     }
                                     if (m_client->supportsConfigurationDone()) {
                                         m_client->configurationDone({});
                                     }
                                     m_handshakeTimer->stop();
                                     if (m_state == State::Starting) {
                                         setState(State::Running);
                                     }
                                 });
    }
}

void DebugSession::stop()
{
    if (m_state == State::Idle && !adapterActive()) {
        return;
    }
    m_stopping = true;
    m_handshakeTimer->stop();
    if (m_client) {
        m_client->disconnectSession(true, {});
    }
    destroyAdapter();
    setState(State::Idle);
    appendConsole(tr("Debug session stopped."));
}

void DebugSession::continueRun()
{
    if (!m_client || m_state != State::Stopped) {
        return;
    }
    m_client->continueDebug(m_threadId, {});
}

void DebugSession::pause()
{
    if (!m_client || m_state != State::Running) {
        return;
    }
    m_client->pause(m_threadId, {});
}

void DebugSession::stepOver()
{
    if (!m_client || m_state != State::Stopped) {
        return;
    }
    m_client->next(m_threadId, {});
}

void DebugSession::stepInto()
{
    if (!m_client || m_state != State::Stopped) {
        return;
    }
    m_client->stepIn(m_threadId, {});
}

void DebugSession::stepOut()
{
    if (!m_client || m_state != State::Stopped) {
        return;
    }
    m_client->stepOut(m_threadId, {});
}

void DebugSession::onContinued()
{
    m_frames.clear();
    clearDebugLine();
    emit stackFramesChanged({});
    emit scopesChanged({});
    setState(State::Running);
}

void DebugSession::onTerminated()
{
    appendConsole(tr("Debug session terminated."));
    destroyAdapter();
    setState(State::Terminated);
    setState(State::Idle);
}

void DebugSession::onStopped(const DapStoppedEvent &event)
{
    m_handshakeTimer->stop();
    m_threadId = event.threadId > 0 ? event.threadId : 1;
    setState(State::Stopped);
    QString line = event.reason;
    if (!event.description.isEmpty()) {
        line += QStringLiteral(": ") + event.description;
    }
    appendConsole(tr("Stopped (%1)").arg(line));
    loadStackAndScopes();
}

void DebugSession::loadStackAndScopes()
{
    if (!m_client) {
        return;
    }
    m_client->stackTrace(m_threadId, [this](const QJsonObject &body, const QJsonObject &error) {
        if (!error.isEmpty()) {
            appendConsole(tr("stackTrace: %1").arg(error.value(QStringLiteral("message")).toString()));
            return;
        }
        m_frames = dapStackFramesFromJson(body);
        emit stackFramesChanged(m_frames);
        if (m_frames.isEmpty()) {
            return;
        }
        selectStackFrame(m_frames.front().id);
    });
}

void DebugSession::selectStackFrame(int frameId)
{
    m_frameId = frameId;
    for (const DapStackFrame &frame : m_frames) {
        if (frame.id != frameId) {
            continue;
        }
        const QString path = dapNormalizePath(frame.sourcePath);
        if (!path.isEmpty() && m_editors) {
            m_editors->revealLocation(path, qMax(0, frame.line - 1), qMax(0, frame.column - 1));
        }
        m_stoppedPath = path;
        m_stoppedLine0 = qMax(-1, frame.line - 1);
        applyEditorDecorations();
        break;
    }
    if (!m_client || frameId <= 0) {
        return;
    }
    m_client->scopes(frameId, [this](const QJsonObject &body, const QJsonObject &error) {
        if (!error.isEmpty()) {
            appendConsole(tr("scopes: %1").arg(error.value(QStringLiteral("message")).toString()));
            emit scopesChanged({});
            return;
        }
        const QVector<DapScope> scopes = dapScopesFromJson(body);
        emit scopesChanged(scopes);
        if (!scopes.isEmpty() && scopes.front().variablesReference > 0) {
            requestVariables(scopes.front().variablesReference);
        }
    });
}

void DebugSession::requestVariables(int variablesReference)
{
    if (!m_client || variablesReference <= 0) {
        return;
    }
    m_client->variables(variablesReference, [this, variablesReference](const QJsonObject &body,
                                                                       const QJsonObject &error) {
        if (!error.isEmpty()) {
            appendConsole(tr("variables: %1").arg(error.value(QStringLiteral("message")).toString()));
            return;
        }
        emit variablesReady(variablesReference, dapVariablesFromJson(body));
    });
}

void DebugSession::evaluate(const QString &expression)
{
    const QString trimmed = expression.trimmed();
    if (trimmed.isEmpty()) {
        return;
    }
    appendConsole(QStringLiteral("> %1").arg(trimmed));
    if (!m_client) {
        appendConsole(tr("No active debug session."));
        return;
    }
    m_client->evaluate(trimmed, m_frameId, [this](const QJsonObject &body, const QJsonObject &error) {
        if (!error.isEmpty()) {
            appendConsole(error.value(QStringLiteral("message")).toString());
            return;
        }
        const QString result = body.value(QStringLiteral("result")).toString();
        appendConsole(result.isEmpty() ? QStringLiteral("ok") : result);
    });
}
