#include "project/WorkspaceSearch.h"

#include "core/AppSettings.h"
#include "core/Logging.h"
#include "project/GitIgnore.h"
#include "project/WorkspaceFileIndex.h"
#include "project/WorkspacePath.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMetaObject>
#include <QTextStream>

CompiledSearch compileSearch(const SearchOptions &options)
{
    CompiledSearch compiled;
    if (options.query.isEmpty()) {
        compiled.error = QStringLiteral("empty");
        return compiled;
    }

    QString pattern = options.regex ? options.query : QRegularExpression::escape(options.query);
    if (options.wholeWord) {
        pattern = QStringLiteral("\\b(?:%1)\\b").arg(pattern);
    }

    QRegularExpression::PatternOptions flags = QRegularExpression::DontCaptureOption;
    if (!options.caseSensitive) {
        flags |= QRegularExpression::CaseInsensitiveOption;
    }
    compiled.regex.setPattern(pattern);
    compiled.regex.setPatternOptions(flags);
    if (!compiled.regex.isValid()) {
        compiled.error = compiled.regex.errorString();
    }
    return compiled;
}

QList<SearchHit> findInText(const QString &text, const QString &path, const CompiledSearch &compiled)
{
    QList<SearchHit> hits;
    if (!compiled.isValid()) {
        return hits;
    }

    int offset = 0;
    int lineNumber = 1;
    while (offset <= text.size()) {
        int end = text.indexOf(QLatin1Char('\n'), offset);
        if (end < 0) {
            end = text.size();
        }
        const QString line = text.mid(offset, end - offset);
        auto it = compiled.regex.globalMatch(line);
        while (it.hasNext()) {
            const QRegularExpressionMatch match = it.next();
            SearchHit hit;
            hit.path = path;
            hit.line = lineNumber;
            hit.column = match.capturedStart();
            hit.length = match.capturedLength();
            hit.lineText = line;
            hits.append(hit);
            if (hits.size() >= 1000) {
                return hits;
            }
        }
        if (end == text.size()) {
            break;
        }
        offset = end + 1;
        ++lineNumber;
    }
    return hits;
}

QString replaceInText(const QString &text,
                      const CompiledSearch &compiled,
                      const QString &replacement,
                      int *count)
{
    if (count) {
        *count = 0;
    }
    if (!compiled.isValid()) {
        return text;
    }
    QString out;
    out.reserve(text.size());
    int last = 0;
    int n = 0;
    auto it = compiled.regex.globalMatch(text);
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        out += text.mid(last, match.capturedStart() - last);
        out += replacement;
        last = match.capturedEnd();
        ++n;
    }
    out += text.mid(last);
    if (count) {
        *count = n;
    }
    return out;
}

namespace {

bool looksBinary(const QByteArray &sample)
{
    return sample.contains('\0');
}

SearchJobResult runSearch(const QString &root,
                          const QStringList &excludedNames,
                          const SearchOptions &options,
                          const QHash<QString, QString> &openBuffers,
                          QAtomicInt *cancel)
{
    SearchJobResult result;
    const CompiledSearch compiled = compileSearch(options);
    if (!compiled.isValid()) {
        result.error = compiled.error;
        return result;
    }

    const GitIgnore gitIgnore = options.useGitIgnore ? GitIgnore::loadFromWorkspace(root) : GitIgnore();
    const qint64 maxBytes = AppSettings().largeFileWarnBytes();
    const QStringList files = collectWorkspaceFiles(root, excludedNames);
    QList<SearchHit> pending;

    auto flush = [&]() {
        result.hits.append(pending);
        pending.clear();
    };

    for (const QString &absolute : files) {
        if (cancel && cancel->loadAcquire()) {
            result.cancelled = true;
            break;
        }
        if (!isInsideWorkspace(root, absolute)) {
            continue;
        }
        const QString relative = QDir::fromNativeSeparators(QDir(root).relativeFilePath(absolute));
        if (relative.startsWith(QLatin1String(".."))) {
            continue;
        }
        if (options.useGitIgnore && gitIgnore.isIgnored(relative, false)) {
            continue;
        }
        if (!globMatches(options.includeGlob, relative)) {
            continue;
        }
        if (!options.excludeGlob.trimmed().isEmpty() && globMatches(options.excludeGlob, relative)) {
            continue;
        }

        QString text;
        if (openBuffers.contains(absolute)) {
            text = openBuffers.value(absolute);
        } else {
            QFileInfo info(absolute);
            if (info.size() > maxBytes && maxBytes > 0) {
                continue;
            }
            QFile file(absolute);
            if (!file.open(QIODevice::ReadOnly)) {
                continue;
            }
            const QByteArray sample = file.peek(8192);
            if (looksBinary(sample)) {
                continue;
            }
            QTextStream stream(&file);
            text = stream.readAll();
            text.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
            text.replace(QLatin1Char('\r'), QLatin1Char('\n'));
        }

        ++result.filesScanned;
        const QList<SearchHit> hits = findInText(text, absolute, compiled);
        result.hitCount += hits.size();
        pending.append(hits);
        if (pending.size() >= 80) {
            flush();
        }
        if (result.hitCount >= 10000) {
            break;
        }
    }
    flush();
    return result;
}

} // namespace

WorkspaceSearch::WorkspaceSearch(QObject *parent)
    : QObject(parent)
{
}

WorkspaceSearch::~WorkspaceSearch()
{
    m_destroying = true;
    m_queued = false;
    m_cancel.storeRelease(1);
    ++m_generation;
    joinWorker();
}

void WorkspaceSearch::joinWorker()
{
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void WorkspaceSearch::start(const QString &root,
                            const QStringList &excludedNames,
                            const SearchOptions &options,
                            const QHash<QString, QString> &openBuffers)
{
    m_request.root = root;
    m_request.excludedNames = excludedNames;
    m_request.options = options;
    m_request.openBuffers = openBuffers;
    if (m_running) {
        m_queued = true;
        m_cancel.storeRelease(1);
        return;
    }
    startJob();
}

void WorkspaceSearch::cancel()
{
    m_queued = false;
    m_cancel.storeRelease(1);
}

bool WorkspaceSearch::isRunning() const
{
    return m_running;
}

void WorkspaceSearch::startJob()
{
    joinWorker();
    m_queued = false;
    m_cancel.storeRelease(0);
    m_running = true;
    const quint64 generation = ++m_generation;
    const Request request = m_request;
    QAtomicInt *cancel = &m_cancel;
    m_thread = std::thread([this, request, cancel, generation]() {
        SearchJobResult result = runSearch(request.root,
                                           request.excludedNames,
                                           request.options,
                                           request.openBuffers,
                                           cancel);
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_handoff = std::move(result);
        }
        QMetaObject::invokeMethod(this, [this, generation]() { applyResult(generation); },
                                  Qt::QueuedConnection);
    });
    qCInfo(lcProject) << "Workspace search started" << request.root;
}

void WorkspaceSearch::applyResult(quint64 generation)
{
    if (m_destroying || generation != m_generation) {
        return;
    }
    SearchJobResult result;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        result = std::move(m_handoff);
    }
    m_running = false;
    if (m_queued) {
        startJob();
        return;
    }
    if (!result.hits.isEmpty()) {
        emit resultsReady(result.hits);
    }
    emit finished(result);
}
