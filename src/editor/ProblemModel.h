#ifndef EDITERAKO_PROBLEMMODEL_H
#define EDITERAKO_PROBLEMMODEL_H

#include "editor/EditorDiagnostic.h"

#include <QHash>
#include <QObject>
#include <QString>
#include <QVector>

struct ProblemItem {
    QString path;
    int line = 0;
    int column = 0;
    EditorDiagnostic::Severity severity = EditorDiagnostic::Severity::Error;
    QString message;
    QString source;
    QString code;
};

class ProblemModel : public QObject
{
    Q_OBJECT

public:
    enum class Filter {
        All,
        Errors,
        Warnings,
    };

    explicit ProblemModel(QObject *parent = nullptr);

    void setFileProblems(const QString &path, const QVector<ProblemItem> &items);
    void setSourceProblems(const QString &source, const QVector<ProblemItem> &items);
    void clearFile(const QString &path);
    void clearAll();
    void setFilter(Filter filter);
    [[nodiscard]] Filter filter() const { return m_filter; }

    [[nodiscard]] QVector<ProblemItem> visibleItems() const;
    [[nodiscard]] int errorCount() const;
    [[nodiscard]] int warningCount() const;
    [[nodiscard]] int informationCount() const;
    [[nodiscard]] int totalCount() const;
    [[nodiscard]] bool isEmpty() const { return m_byFile.isEmpty(); }

signals:
    void changed();

private:
    [[nodiscard]] bool matchesFilter(const ProblemItem &item) const;
    void recount();

    QHash<QString, QVector<ProblemItem>> m_byFile;
    Filter m_filter = Filter::All;
    int m_errors = 0;
    int m_warnings = 0;
    int m_infos = 0;
};

#endif
