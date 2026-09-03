#ifndef EDITERAKO_GITIGNORE_H
#define EDITERAKO_GITIGNORE_H

#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <QVector>

class GitIgnore
{
public:
    struct Rule {
        bool negation = false;
        bool directoryOnly = false;
        QRegularExpression regex;
    };

    [[nodiscard]] static GitIgnore fromText(const QString &text);
    [[nodiscard]] static GitIgnore loadFromWorkspace(const QString &workspaceRoot);

    [[nodiscard]] bool isIgnored(const QString &relativePath, bool isDirectory) const;
    [[nodiscard]] int ruleCount() const { return m_rules.size(); }

private:
    QVector<Rule> m_rules;
};

[[nodiscard]] bool globMatches(const QString &pattern, const QString &relativePath);

#endif
