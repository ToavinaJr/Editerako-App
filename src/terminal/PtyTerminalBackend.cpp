#include "terminal/PtyTerminalBackend.h"

#include "core/Logging.h"

#include <QDir>
#include <QTimer>
#include <QtGlobal>
#include <string>
#include <vector>

#ifdef Q_OS_WIN
#include <windows.h>
#include <cstring>
#include <QWinEventNotifier>
#ifndef HPCON
typedef VOID *HPCON;
#endif
#ifndef PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE
#define PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE 0x00020016
#endif
#else
#include <cerrno>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <unistd.h>
#include <QSocketNotifier>
#endif

struct PtyTerminalBackend::Impl {
#ifdef Q_OS_WIN
    HANDLE inWrite = INVALID_HANDLE_VALUE;
    HANDLE outRead = INVALID_HANDLE_VALUE;
    HANDLE process = INVALID_HANDLE_VALUE;
    HANDLE thread = INVALID_HANDLE_VALUE;
    HPCON console = nullptr;
    QTimer *poll = nullptr;
    QWinEventNotifier *exitNotifier = nullptr;
    using CreateFn = HRESULT(WINAPI *)(COORD, HANDLE, HANDLE, DWORD, HPCON *);
    using CloseFn = void(WINAPI *)(HPCON);
    using ResizeFn = HRESULT(WINAPI *)(HPCON, COORD);
    CreateFn create = nullptr;
    CloseFn close = nullptr;
    ResizeFn resizeFn = nullptr;
#else
    int master = -1;
    pid_t pid = -1;
    QSocketNotifier *notifier = nullptr;
    QTimer *reaper = nullptr;
#endif
    bool running = false;
    bool stopping = false;
    int columns = 80;
    int rows = 24;
};

#ifdef Q_OS_WIN

namespace {

QString quoteWinArg(const QString &arg)
{
    if (!arg.contains(QLatin1Char(' ')) && !arg.contains(QLatin1Char('"'))) {
        return arg;
    }
    QString quoted = QLatin1Char('"') + arg + QLatin1Char('"');
    return quoted;
}

template<typename Fn>
Fn loadKernelProc(const char *name)
{
    Fn fn{};
    HMODULE kernel = GetModuleHandleW(L"kernel32");
    if (!kernel) {
        return fn;
    }
    const FARPROC raw = GetProcAddress(kernel, name);
    std::memcpy(&fn, &raw, sizeof(raw));
    return fn;
}

} // namespace

#endif

PtyTerminalBackend::PtyTerminalBackend(QObject *parent)
    : ITerminalBackend(parent)
    , m_impl(new Impl)
{
#ifdef Q_OS_WIN
    m_impl->create = loadKernelProc<Impl::CreateFn>("CreatePseudoConsole");
    m_impl->close = loadKernelProc<Impl::CloseFn>("ClosePseudoConsole");
    m_impl->resizeFn = loadKernelProc<Impl::ResizeFn>("ResizePseudoConsole");
    m_impl->poll = new QTimer(this);
    m_impl->poll->setInterval(20);
    connect(m_impl->poll, &QTimer::timeout, this, [this]() {
        if (!m_impl || m_impl->stopping || m_impl->outRead == INVALID_HANDLE_VALUE) {
            return;
        }
        DWORD available = 0;
        if (!PeekNamedPipe(m_impl->outRead, nullptr, 0, nullptr, &available, nullptr)) {
            const DWORD err = GetLastError();
            if ((err == ERROR_BROKEN_PIPE || err == ERROR_INVALID_HANDLE) && m_impl->running) {
                DWORD code = 0;
                if (m_impl->process != INVALID_HANDLE_VALUE) {
                    GetExitCodeProcess(m_impl->process, &code);
                }
                m_impl->running = false;
                m_impl->poll->stop();
                if (m_impl->exitNotifier) {
                    m_impl->exitNotifier->setEnabled(false);
                }
                emit finished(static_cast<int>(code), false);
            }
            return;
        }
        if (available > 0) {
            QByteArray buffer;
            buffer.resize(static_cast<int>(available));
            DWORD read = 0;
            if (ReadFile(m_impl->outRead, buffer.data(), available, &read, nullptr) && read > 0) {
                buffer.resize(static_cast<int>(read));
                emit dataReceived(buffer, false);
            }
        }
        if (m_impl->process != INVALID_HANDLE_VALUE
            && WaitForSingleObject(m_impl->process, 0) == WAIT_OBJECT_0 && m_impl->running) {
            DWORD code = 0;
            GetExitCodeProcess(m_impl->process, &code);
            m_impl->running = false;
            m_impl->poll->stop();
            if (m_impl->exitNotifier) {
                m_impl->exitNotifier->setEnabled(false);
            }
            emit finished(static_cast<int>(code), false);
        }
    });
#else
    m_impl->reaper = new QTimer(this);
    m_impl->reaper->setInterval(50);
    connect(m_impl->reaper, &QTimer::timeout, this, [this]() {
        if (!m_impl || m_impl->pid <= 0 || m_impl->stopping) {
            return;
        }
        int status = 0;
        const pid_t result = waitpid(m_impl->pid, &status, WNOHANG);
        if (result == m_impl->pid) {
            m_impl->running = false;
            m_impl->reaper->stop();
            const int code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
            emit finished(code, WIFSIGNALED(status));
        }
    });
#endif
}

PtyTerminalBackend::~PtyTerminalBackend()
{
    stop();
    delete m_impl;
}

bool PtyTerminalBackend::isAvailable()
{
#ifdef Q_OS_WIN
    HMODULE kernel = GetModuleHandleW(L"kernel32");
    return kernel && GetProcAddress(kernel, "CreatePseudoConsole") != nullptr;
#else
    return true;
#endif
}

void PtyTerminalBackend::start(const QString &program, const QStringList &arguments,
                               const QString &workingDirectory, int columns, int rows)
{
    if (!m_impl || m_impl->running || program.isEmpty() || !isAvailable()) {
        emit failed(tr("PTY is not available"));
        return;
    }
    resize(columns, rows);
    const QString cwd = workingDirectory.isEmpty() ? QDir::currentPath() : workingDirectory;

#ifdef Q_OS_WIN
    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    HANDLE inRead = INVALID_HANDLE_VALUE;
    HANDLE outWrite = INVALID_HANDLE_VALUE;
    if (!CreatePipe(&inRead, &m_impl->inWrite, &sa, 0)
        || !CreatePipe(&m_impl->outRead, &outWrite, &sa, 0)) {
        emit failed(tr("Failed to create PTY pipes"));
        return;
    }
    SetHandleInformation(m_impl->inWrite, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(m_impl->outRead, HANDLE_FLAG_INHERIT, 0);
    COORD size{static_cast<SHORT>(qMax(20, m_impl->columns)),
               static_cast<SHORT>(qMax(5, m_impl->rows))};
    if (FAILED(m_impl->create(size, inRead, outWrite, 0, &m_impl->console))) {
        emit failed(tr("CreatePseudoConsole failed"));
        return;
    }
    CloseHandle(inRead);
    CloseHandle(outWrite);

    SIZE_T bytes = 0;
    InitializeProcThreadAttributeList(nullptr, 1, 0, &bytes);
    auto *attrs = static_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(HeapAlloc(GetProcessHeap(), 0, bytes));
    if (!attrs || !InitializeProcThreadAttributeList(attrs, 1, 0, &bytes)) {
        emit failed(tr("Failed to init process attributes"));
        return;
    }
    UpdateProcThreadAttribute(attrs, 0, PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE, m_impl->console,
                              sizeof(m_impl->console), nullptr, nullptr);
    STARTUPINFOEXW si{};
    si.StartupInfo.cb = sizeof(si);
    si.lpAttributeList = attrs;
    QString cmd = quoteWinArg(program);
    for (const QString &arg : arguments) {
        cmd += QLatin1Char(' ') + quoteWinArg(arg);
    }
    std::wstring cmdLine = cmd.toStdWString();
    PROCESS_INFORMATION pi{};
    const BOOL ok = CreateProcessW(
        nullptr, cmdLine.data(), nullptr, nullptr, FALSE, EXTENDED_STARTUPINFO_PRESENT, nullptr,
        cwd.isEmpty() ? nullptr : reinterpret_cast<LPCWSTR>(cwd.utf16()), &si.StartupInfo, &pi);
    DeleteProcThreadAttributeList(attrs);
    HeapFree(GetProcessHeap(), 0, attrs);
    if (!ok) {
        emit failed(tr("Failed to start PTY process"));
        return;
    }
    m_impl->process = pi.hProcess;
    m_impl->thread = pi.hThread;
    m_impl->running = true;
    delete m_impl->exitNotifier;
    m_impl->exitNotifier = new QWinEventNotifier(m_impl->process, this);
    connect(m_impl->exitNotifier, &QWinEventNotifier::activated, this, [this]() {
        if (!m_impl || m_impl->stopping || !m_impl->running) {
            return;
        }
        DWORD available = 0;
        if (m_impl->outRead != INVALID_HANDLE_VALUE
            && PeekNamedPipe(m_impl->outRead, nullptr, 0, nullptr, &available, nullptr)
            && available > 0) {
            QByteArray buffer;
            buffer.resize(static_cast<int>(available));
            DWORD read = 0;
            if (ReadFile(m_impl->outRead, buffer.data(), available, &read, nullptr) && read > 0) {
                buffer.resize(static_cast<int>(read));
                emit dataReceived(buffer, false);
            }
        }
        DWORD code = 0;
        GetExitCodeProcess(m_impl->process, &code);
        m_impl->running = false;
        m_impl->poll->stop();
        m_impl->exitNotifier->setEnabled(false);
        emit finished(static_cast<int>(code), false);
    });
    m_impl->poll->start();
    qCInfo(lcTerminal) << "ConPTY backend" << program << arguments;
#else
    m_impl->master = posix_openpt(O_RDWR | O_NOCTTY);
    if (m_impl->master < 0 || grantpt(m_impl->master) != 0 || unlockpt(m_impl->master) != 0) {
        emit failed(tr("Failed to open PTY"));
        return;
    }
    const char *slaveName = ptsname(m_impl->master);
    std::vector<std::string> argStorage;
    argStorage.push_back(program.toLocal8Bit().toStdString());
    for (const QString &arg : arguments) {
        argStorage.push_back(arg.toLocal8Bit().toStdString());
    }
    std::vector<char *> argv;
    argv.reserve(argStorage.size() + 1);
    for (std::string &item : argStorage) {
        argv.push_back(item.data());
    }
    argv.push_back(nullptr);
    const std::string cwdUtf8 = cwd.toLocal8Bit().toStdString();
    const pid_t pid = fork();
    if (pid < 0) {
        emit failed(tr("fork failed"));
        return;
    }
    if (pid == 0) {
        setsid();
        const int slave = open(slaveName, O_RDWR);
        if (slave < 0) {
            _exit(127);
        }
        ioctl(slave, TIOCSCTTY, 0);
        dup2(slave, STDIN_FILENO);
        dup2(slave, STDOUT_FILENO);
        dup2(slave, STDERR_FILENO);
        if (slave > STDERR_FILENO) {
            close(slave);
        }
        close(m_impl->master);
        if (!cwdUtf8.empty()) {
            chdir(cwdUtf8.c_str());
        }
        execvp(argv[0], argv.data());
        _exit(127);
    }
    m_impl->pid = pid;
    m_impl->running = true;
    m_impl->notifier = new QSocketNotifier(m_impl->master, QSocketNotifier::Read, this);
    connect(m_impl->notifier, &QSocketNotifier::activated, this, [this]() {
        if (!m_impl || m_impl->master < 0) {
            return;
        }
        char buffer[4096];
        const ssize_t n = ::read(m_impl->master, buffer, sizeof(buffer));
        if (n > 0) {
            emit dataReceived(QByteArray(buffer, static_cast<int>(n)), false);
        }
    });
    m_impl->reaper->start();
    resize(m_impl->columns, m_impl->rows);
    qCInfo(lcTerminal) << "PTY backend" << program << arguments;
#endif
}

void PtyTerminalBackend::write(const QByteArray &data)
{
    if (!m_impl || data.isEmpty() || !m_impl->running) {
        return;
    }
#ifdef Q_OS_WIN
    DWORD written = 0;
    WriteFile(m_impl->inWrite, data.constData(), static_cast<DWORD>(data.size()), &written, nullptr);
#else
    if (m_impl->master >= 0) {
        ::write(m_impl->master, data.constData(), static_cast<size_t>(data.size()));
    }
#endif
}

void PtyTerminalBackend::resize(int columns, int rows)
{
    if (!m_impl) {
        return;
    }
    if (columns > 0) {
        m_impl->columns = columns;
    }
    if (rows > 0) {
        m_impl->rows = rows;
    }
#ifdef Q_OS_WIN
    if (m_impl->console && m_impl->resizeFn) {
        COORD size{static_cast<SHORT>(m_impl->columns), static_cast<SHORT>(m_impl->rows)};
        m_impl->resizeFn(m_impl->console, size);
    }
#else
    if (m_impl->master >= 0) {
        struct winsize ws {};
        ws.ws_col = static_cast<unsigned short>(m_impl->columns);
        ws.ws_row = static_cast<unsigned short>(m_impl->rows);
        ioctl(m_impl->master, TIOCSWINSZ, &ws);
    }
#endif
}

void PtyTerminalBackend::stop()
{
    if (!m_impl || m_impl->stopping) {
        return;
    }
    m_impl->stopping = true;
    m_impl->running = false;
#ifdef Q_OS_WIN
    if (m_impl->poll) {
        m_impl->poll->stop();
    }
    if (m_impl->exitNotifier) {
        m_impl->exitNotifier->setEnabled(false);
        delete m_impl->exitNotifier;
        m_impl->exitNotifier = nullptr;
    }
    if (m_impl->process != INVALID_HANDLE_VALUE) {
        TerminateProcess(m_impl->process, 1);
        CloseHandle(m_impl->process);
        m_impl->process = INVALID_HANDLE_VALUE;
    }
    if (m_impl->thread != INVALID_HANDLE_VALUE) {
        CloseHandle(m_impl->thread);
        m_impl->thread = INVALID_HANDLE_VALUE;
    }
    if (m_impl->console && m_impl->close) {
        m_impl->close(m_impl->console);
        m_impl->console = nullptr;
    }
    if (m_impl->inWrite != INVALID_HANDLE_VALUE) {
        CloseHandle(m_impl->inWrite);
        m_impl->inWrite = INVALID_HANDLE_VALUE;
    }
    if (m_impl->outRead != INVALID_HANDLE_VALUE) {
        CloseHandle(m_impl->outRead);
        m_impl->outRead = INVALID_HANDLE_VALUE;
    }
#else
    if (m_impl->reaper) {
        m_impl->reaper->stop();
    }
    delete m_impl->notifier;
    m_impl->notifier = nullptr;
    if (m_impl->pid > 0) {
        kill(m_impl->pid, SIGTERM);
        m_impl->pid = -1;
    }
    if (m_impl->master >= 0) {
        close(m_impl->master);
        m_impl->master = -1;
    }
#endif
    m_impl->stopping = false;
}

bool PtyTerminalBackend::isRunning() const
{
    return m_impl && m_impl->running;
}
