#ifndef EDITERAKO_DEBUGSESSION_H
#define EDITERAKO_DEBUGSESSION_H

#include "debug/BreakpointStore.h"
#include "debug/DapTypes.h"
#include "debug/LaunchFile.h"

#include <QObject>
#include <QString>
#include <QVector>

class CodeEditor;
class DapClient;
class EditorManager;
class LspServerProcess;
class QTimer;

class DebugSession : public QObject
{
    Q_OBJECT

public:
    enum class State {
        Idle,
        Starting,
        Running,
        Stopped,
        Terminated,
    };
    Q_ENUM(State)

    DebugSession(EditorManager *editors, QObject *parent = nullptr);
    ~DebugSession() override;

    void setWorkspaceRoot(const QString &root);
    [[nodiscard]] QString workspaceRoot() const { return m_workspaceRoot; }

    [[nodiscard]] State state() const { return m_state; }
    [[nodiscard]] BreakpointStore *breakpoints() { return &m_breakpoints; }
    [[nodiscard]] QVector<LaunchConfiguration> configurations() const { return m_configurations; }
    [[nodiscard]] QString selectedConfiguration() const { return m_selectedName; }
    void setSelectedConfiguration(const QString &name);

    void start();
    void stop();
    void continueRun();
    void pause();
    void stepOver();
    void stepInto();
    void stepOut();
    void toggleBreakpointAtCursor();
    void toggleBreakpoint(const QString &path, int line0);
    void selectStackFrame(int frameId);
    void requestVariables(int variablesReference);
    void evaluate(const QString &expression);
    bool createLaunchFile(QString *error = nullptr);
    void reloadConfigurations();

signals:
    void stateChanged(DebugSession::State state);
    void statusMessage(const QString &message, int timeoutMs);
    void debugStatusChanged(const QString &text);
    void outputReceived(const QString &category, const QString &text);
    void stackFramesChanged(const QVector<DapStackFrame> &frames);
    void scopesChanged(const QVector<DapScope> &scopes);
    void variablesReady(int variablesReference, const QVector<DapVariable> &variables);
    void configurationsChanged();
    void panelRevealRequested();

private:
    void onDocumentOpened(CodeEditor *editor);
    void attachEditor(CodeEditor *editor);
    void applyEditorDecorations();
    void applyEditorDecorations(CodeEditor *editor);
    void clearDebugLine();
    void setState(State state);
    void appendConsole(const QString &text);
    void failStart(const QString &message);
    void destroyAdapter();
    void beginHandshake(const LaunchConfiguration &config);
    void sendLaunchOrAttach(const LaunchConfiguration &config);
    void syncBreakpointsThenConfigDone();
    void onInitializedEvent();
    void onStopped(const DapStoppedEvent &event);
    void onContinued();
    void onTerminated();
    void loadStackAndScopes();
    [[nodiscard]] LaunchConfiguration selectedOrFirst() const;
    [[nodiscard]] LaunchContext currentContext() const;
    [[nodiscard]] QString currentFilePath() const;
    [[nodiscard]] bool adapterActive() const;

    EditorManager *m_editors = nullptr;
    LspServerProcess *m_process = nullptr;
    DapClient *m_client = nullptr;
    QTimer *m_handshakeTimer = nullptr;
    BreakpointStore m_breakpoints;
    QVector<LaunchConfiguration> m_configurations;
    QString m_workspaceRoot;
    QString m_selectedName;
    QVector<DapStackFrame> m_frames;
    QString m_stoppedPath;
    int m_stoppedLine0 = -1;
    int m_threadId = 1;
    int m_frameId = 0;
    int m_pendingBreakpointSets = 0;
    bool m_stopping = false;
    State m_state = State::Idle;
};

#endif
