#ifndef EDITERAKO_LANGUAGEREGISTRY_H
#define EDITERAKO_LANGUAGEREGISTRY_H

#include <QByteArray>
#include <QList>
#include <QString>
#include <QStringList>

extern "C" {
struct TSLanguage;
}

enum class LanguageId {
    PlainText,
    C,
    Cpp,
    CMake,
    Html,
    Css,
    JavaScript,
    TypeScript,
    Tsx,
    Json,
    Markdown,
    Python,
    Shell,
    Sql,
    Yaml,
};

struct CommentTokens {
    QString line;
    QString blockOpen;
    QString blockClose;
};

struct LanguageDefinition {
    LanguageId id = LanguageId::PlainText;
    QString displayName;
    QStringList extensions;
    QStringList filenames;
    const TSLanguage *(*treeSitterLanguage)() = nullptr;
    QString highlightQueryResourcePath;
    CommentTokens commentTokens;
    QString brackets;
    QString indentTriggers;
    QString languageServer;
};

class LanguageRegistry
{
public:
    [[nodiscard]] static const QList<LanguageDefinition> &all();
    [[nodiscard]] static const LanguageDefinition &definition(LanguageId id);

    [[nodiscard]] static LanguageId idForPath(const QString &path);
    [[nodiscard]] static LanguageId idForExtension(const QString &extension);
    [[nodiscard]] static QString displayName(LanguageId id);
    [[nodiscard]] static const TSLanguage *tsLanguage(LanguageId id);
    [[nodiscard]] static QString highlightQueryResourcePath(LanguageId id);
    [[nodiscard]] static QByteArray highlightQuerySource(LanguageId id);
    [[nodiscard]] static CommentTokens commentTokens(LanguageId id);
};

#endif
