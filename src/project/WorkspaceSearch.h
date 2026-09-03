#ifndef EDITERAKO_WORKSPACESEARCH_H
#define EDITERAKO_WORKSPACESEARCH_H

#include <QAtomicInt>
#include <QFutureWatcher>
#include <QHash>
#include <QList>
#include <QObject>
#include <QRegularExpression>
#include <QString>
#include <QStringList>

struct SearchOptions {
    QString query;
    QString replacement;
    bool regex = false;
    bool caseSensitive = false;
    bool wholeWord = false;
    QString includeGlob;
    QString excludeGlob;
    bool useGitIgnore = true;
};

struct SearchHit {
    QString path;
    int line = 1;
    int column = 0;
    int length = 0;
    QString lineText;
};

struct CompiledSearch {
    QRegularExpression regex;
    QString error;
    [[nodiscard]] bool isValid() const { return error.isEmpty() && regex.isValid(); }
};

struct SearchJobResult {
    QList<SearchHit> hits;
    int filesScanned = 0;
    int hitCount = 0;
    bool cancelled = false;
    QString error;
};

[[nodiscard]] CompiledSearch compileSearch(const SearchOptions &options);
[[nodiscard]] QList<SearchHit> findInText(const QString &text,
                                          const QString &path,
                                          const CompiledSearch &compiled);
[[nodiscard]] QString replaceInText(const QString &text,
                                    const CompiledSearch &compiled,
                                    const QString &replacement,
                                    int *count = nullptr);

class WorkspaceSearch : public QObject
{
    Q_OBJECT

public:
    explicit WorkspaceSearch(QObject *parent = nullptr);
    ~WorkspaceSearch() override;

    void start(const QString &root,
               const QStringList &excludedNames,
               const SearchOptions &options,
               const QHash<QString, QString> &openBuffers = {});
    void cancel();
    [[nodiscard]] bool isRunning() const;

signals:
    void resultsReady(const QList<SearchHit> &hits);
    void finished(const SearchJobResult &result);

private:
    void startJob();

    struct Request {
        QString root;
        QStringList excludedNames;
        SearchOptions options;
        QHash<QString, QString> openBuffers;
    };

    Request m_request;
    QFutureWatcher<SearchJobResult> m_watcher;
    QAtomicInt m_cancel;
    bool m_queued = false;
    bool m_destroying = false;
};

#endif
