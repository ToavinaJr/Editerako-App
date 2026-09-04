#include "tasks/ProblemMatcher.h"

#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>

namespace {

QString resolveReportedPath(const QString &reported, const QString &workspace,
                            const QString &workingDirectory)
{
    const QString trimmed = QDir::fromNativeSeparators(reported.trimmed());
    if (trimmed.isEmpty()) {
        return {};
    }
    const QFileInfo info(trimmed);
    if (info.isAbsolute()) {
        return QDir::cleanPath(trimmed);
    }
    const QString base = workingDirectory.isEmpty() ? workspace : workingDirectory;
    if (base.isEmpty()) {
        return QDir::cleanPath(trimmed);
    }
    return QDir::cleanPath(QDir(base).absoluteFilePath(trimmed));
}

TaskProblem::Severity severityFrom(const QString &text)
{
    const QString lower = text.toLower();
    if (lower.contains(QLatin1String("warning"))) {
        return TaskProblem::Severity::Warning;
    }
    if (lower.contains(QLatin1String("note")) || lower.contains(QLatin1String("info"))) {
        return TaskProblem::Severity::Information;
    }
    return TaskProblem::Severity::Error;
}

QVector<TaskProblem> matchGcc(const QString &output, const QString &workspace,
                              const QString &workingDirectory)
{
    static const QRegularExpression re(
        QStringLiteral(R"(^(.+?):(\d+)(?::(\d+))?:\s+(fatal error|error|warning|note):\s+(.*)$)"),
        QRegularExpression::MultilineOption);
    QVector<TaskProblem> out;
    auto it = re.globalMatch(output);
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        TaskProblem problem;
        problem.path = resolveReportedPath(match.captured(1), workspace, workingDirectory);
        problem.line = match.captured(2).toInt();
        problem.column = match.captured(3).toInt();
        problem.severity = severityFrom(match.captured(4));
        problem.message = match.captured(5).trimmed();
        if (!problem.path.isEmpty()) {
            out.append(problem);
        }
    }
    return out;
}

QVector<TaskProblem> matchMsvc(const QString &output, const QString &workspace,
                               const QString &workingDirectory)
{
    static const QRegularExpression re(
        QStringLiteral(
            R"(^(.+?)\((\d+)(?:,(\d+))?\)\s*:\s+(fatal error|error|warning|note)\s+[^:]+:\s+(.*)$)"),
        QRegularExpression::MultilineOption);
    QVector<TaskProblem> out;
    auto it = re.globalMatch(output);
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        TaskProblem problem;
        problem.path = resolveReportedPath(match.captured(1), workspace, workingDirectory);
        problem.line = match.captured(2).toInt();
        problem.column = match.captured(3).toInt();
        problem.severity = severityFrom(match.captured(4));
        problem.message = match.captured(5).trimmed();
        if (!problem.path.isEmpty()) {
            out.append(problem);
        }
    }
    return out;
}

} // namespace

QVector<TaskProblem> matchTaskProblems(const QString &output, const QString &matcherId,
                                       const QString &workspaceRoot,
                                       const QString &workingDirectory)
{
    QString id = matcherId.trimmed().toLower();
    if (id.startsWith(QLatin1Char('$'))) {
        id = id.mid(1);
    }
    if (id == QLatin1String("msvc")) {
        return matchMsvc(output, workspaceRoot, workingDirectory);
    }
    if (id.isEmpty() || id == QLatin1String("gcc") || id == QLatin1String("gnu")) {
        return matchGcc(output, workspaceRoot, workingDirectory);
    }
    return {};
}
