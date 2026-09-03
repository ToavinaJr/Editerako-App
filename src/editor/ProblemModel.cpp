#include "editor/ProblemModel.h"

#include <algorithm>

ProblemModel::ProblemModel(QObject *parent)
    : QObject(parent)
{
}

void ProblemModel::setFileProblems(const QString &path, const QVector<ProblemItem> &items)
{
    if (path.isEmpty()) {
        return;
    }
    if (items.isEmpty()) {
        if (!m_byFile.remove(path)) {
            return;
        }
    } else {
        m_byFile.insert(path, items);
    }
    recount();
    emit changed();
}

void ProblemModel::clearFile(const QString &path)
{
    setFileProblems(path, {});
}

void ProblemModel::clearAll()
{
    if (m_byFile.isEmpty()) {
        return;
    }
    m_byFile.clear();
    recount();
    emit changed();
}

void ProblemModel::setFilter(Filter filter)
{
    if (m_filter == filter) {
        return;
    }
    m_filter = filter;
    emit changed();
}

bool ProblemModel::matchesFilter(const ProblemItem &item) const
{
    switch (m_filter) {
    case Filter::Errors:
        return item.severity == EditorDiagnostic::Severity::Error;
    case Filter::Warnings:
        return item.severity == EditorDiagnostic::Severity::Warning;
    case Filter::All:
        break;
    }
    return true;
}

QVector<ProblemItem> ProblemModel::visibleItems() const
{
    QVector<ProblemItem> out;
    for (auto it = m_byFile.constBegin(); it != m_byFile.constEnd(); ++it) {
        for (const ProblemItem &item : it.value()) {
            if (matchesFilter(item)) {
                out.append(item);
            }
        }
    }
    std::sort(out.begin(), out.end(), [](const ProblemItem &a, const ProblemItem &b) {
        const int pathCmp = QString::compare(a.path, b.path, Qt::CaseInsensitive);
        if (pathCmp != 0) {
            return pathCmp < 0;
        }
        if (a.line != b.line) {
            return a.line < b.line;
        }
        if (a.column != b.column) {
            return a.column < b.column;
        }
        return static_cast<int>(a.severity) < static_cast<int>(b.severity);
    });
    return out;
}

int ProblemModel::errorCount() const
{
    return m_errors;
}

int ProblemModel::warningCount() const
{
    return m_warnings;
}

int ProblemModel::informationCount() const
{
    return m_infos;
}

int ProblemModel::totalCount() const
{
    return m_errors + m_warnings + m_infos;
}

void ProblemModel::recount()
{
    m_errors = 0;
    m_warnings = 0;
    m_infos = 0;
    for (auto it = m_byFile.constBegin(); it != m_byFile.constEnd(); ++it) {
        for (const ProblemItem &item : it.value()) {
            switch (item.severity) {
            case EditorDiagnostic::Severity::Error:
                ++m_errors;
                break;
            case EditorDiagnostic::Severity::Warning:
                ++m_warnings;
                break;
            case EditorDiagnostic::Severity::Information:
            case EditorDiagnostic::Severity::Hint:
                ++m_infos;
                break;
            }
        }
    }
}
