#ifndef EDITERAKO_LANGUAGEREGISTRY_H
#define EDITERAKO_LANGUAGEREGISTRY_H

#include <QByteArray>
#include <QString>

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

class LanguageRegistry
{
public:
    [[nodiscard]] static LanguageId idForPath(const QString &path);
    [[nodiscard]] static LanguageId idForExtension(const QString &extension);
    [[nodiscard]] static QString displayName(LanguageId id);
    [[nodiscard]] static const TSLanguage *tsLanguage(LanguageId id);
    [[nodiscard]] static QString highlightQueryResourcePath(LanguageId id);
    [[nodiscard]] static QByteArray highlightQuerySource(LanguageId id);
    [[nodiscard]] static CommentTokens commentTokens(LanguageId id);
};

#endif
