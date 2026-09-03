#include "syntax/LanguageRegistry.h"

#include <QFile>
#include <QFileInfo>

extern "C" {
struct TSLanguage;
TSLanguage *tree_sitter_cpp();
TSLanguage *tree_sitter_html();
}

LanguageId LanguageRegistry::idForPath(const QString &path)
{
    if (path.isEmpty()) {
        return LanguageId::PlainText;
    }

    const QFileInfo info(path);
    const QString name = info.fileName();
    if (name.compare(QLatin1String("CMakeLists.txt"), Qt::CaseInsensitive) == 0) {
        return LanguageId::CMake;
    }
    return idForExtension(info.suffix());
}

LanguageId LanguageRegistry::idForExtension(const QString &extension)
{
    const QString ext = extension.toLower();
    if (ext == QLatin1String("c")) {
        return LanguageId::C;
    }
    if (ext == QLatin1String("cpp") || ext == QLatin1String("cc") || ext == QLatin1String("cxx")
        || ext == QLatin1String("c++") || ext == QLatin1String("h") || ext == QLatin1String("hpp")
        || ext == QLatin1String("hh") || ext == QLatin1String("hxx")) {
        return LanguageId::Cpp;
    }
    if (ext == QLatin1String("cmake")) {
        return LanguageId::CMake;
    }
    if (ext == QLatin1String("html") || ext == QLatin1String("htm")) {
        return LanguageId::Html;
    }
    if (ext == QLatin1String("css")) {
        return LanguageId::Css;
    }
    if (ext == QLatin1String("js") || ext == QLatin1String("mjs") || ext == QLatin1String("cjs")) {
        return LanguageId::JavaScript;
    }
    if (ext == QLatin1String("ts")) {
        return LanguageId::TypeScript;
    }
    if (ext == QLatin1String("tsx")) {
        return LanguageId::Tsx;
    }
    if (ext == QLatin1String("json")) {
        return LanguageId::Json;
    }
    if (ext == QLatin1String("md") || ext == QLatin1String("markdown")) {
        return LanguageId::Markdown;
    }
    if (ext == QLatin1String("py")) {
        return LanguageId::Python;
    }
    if (ext == QLatin1String("sh") || ext == QLatin1String("bash") || ext == QLatin1String("zsh")) {
        return LanguageId::Shell;
    }
    if (ext == QLatin1String("sql")) {
        return LanguageId::Sql;
    }
    if (ext == QLatin1String("yml") || ext == QLatin1String("yaml")) {
        return LanguageId::Yaml;
    }
    return LanguageId::PlainText;
}

QString LanguageRegistry::displayName(LanguageId id)
{
    switch (id) {
    case LanguageId::C:
        return QStringLiteral("C");
    case LanguageId::Cpp:
        return QStringLiteral("C++");
    case LanguageId::CMake:
        return QStringLiteral("CMake");
    case LanguageId::Html:
        return QStringLiteral("HTML");
    case LanguageId::Css:
        return QStringLiteral("CSS");
    case LanguageId::JavaScript:
        return QStringLiteral("JavaScript");
    case LanguageId::TypeScript:
        return QStringLiteral("TypeScript");
    case LanguageId::Tsx:
        return QStringLiteral("TSX");
    case LanguageId::Json:
        return QStringLiteral("JSON");
    case LanguageId::Markdown:
        return QStringLiteral("Markdown");
    case LanguageId::Python:
        return QStringLiteral("Python");
    case LanguageId::Shell:
        return QStringLiteral("Shell");
    case LanguageId::Sql:
        return QStringLiteral("SQL");
    case LanguageId::Yaml:
        return QStringLiteral("YAML");
    case LanguageId::PlainText:
        break;
    }
    return QStringLiteral("Plain Text");
}

const TSLanguage *LanguageRegistry::tsLanguage(LanguageId id)
{
    switch (id) {
    case LanguageId::C:
    case LanguageId::Cpp:
        return tree_sitter_cpp();
    case LanguageId::Html:
        return tree_sitter_html();
    case LanguageId::PlainText:
    case LanguageId::CMake:
    case LanguageId::Css:
    case LanguageId::JavaScript:
    case LanguageId::TypeScript:
    case LanguageId::Tsx:
    case LanguageId::Json:
    case LanguageId::Markdown:
    case LanguageId::Python:
    case LanguageId::Shell:
    case LanguageId::Sql:
    case LanguageId::Yaml:
        break;
    }
    return nullptr;
}

QString LanguageRegistry::highlightQueryResourcePath(LanguageId id)
{
    switch (id) {
    case LanguageId::C:
    case LanguageId::Cpp:
        return QStringLiteral(":/editerako/syntax/cpp/highlights.scm");
    case LanguageId::Html:
        return QStringLiteral(":/editerako/syntax/html/highlights.scm");
    case LanguageId::PlainText:
    case LanguageId::CMake:
    case LanguageId::Css:
    case LanguageId::JavaScript:
    case LanguageId::TypeScript:
    case LanguageId::Tsx:
    case LanguageId::Json:
    case LanguageId::Markdown:
    case LanguageId::Python:
    case LanguageId::Shell:
    case LanguageId::Sql:
    case LanguageId::Yaml:
        break;
    }
    return {};
}

QByteArray LanguageRegistry::highlightQuerySource(LanguageId id)
{
    const QString path = highlightQueryResourcePath(id);
    if (path.isEmpty()) {
        return {};
    }
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    return file.readAll();
}

CommentTokens LanguageRegistry::commentTokens(LanguageId id)
{
    switch (id) {
    case LanguageId::C:
    case LanguageId::Cpp:
    case LanguageId::JavaScript:
    case LanguageId::TypeScript:
    case LanguageId::Tsx:
        return {QStringLiteral("//"), QStringLiteral("/*"), QStringLiteral("*/")};
    case LanguageId::Css:
        return {{}, QStringLiteral("/*"), QStringLiteral("*/")};
    case LanguageId::Html:
    case LanguageId::Markdown:
        return {{}, QStringLiteral("<!--"), QStringLiteral("-->")};
    case LanguageId::CMake:
    case LanguageId::Python:
    case LanguageId::Shell:
    case LanguageId::Yaml:
        return {QStringLiteral("#"), {}, {}};
    case LanguageId::Sql:
        return {QStringLiteral("--"), QStringLiteral("/*"), QStringLiteral("*/")};
    case LanguageId::Json:
    case LanguageId::PlainText:
        break;
    }
    return {};
}
