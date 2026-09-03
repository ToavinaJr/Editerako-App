#include "syntax/LanguageRegistry.h"

#include <QFile>
#include <QFileInfo>

extern "C" {
const TSLanguage *tree_sitter_c(void);
const TSLanguage *tree_sitter_cpp(void);
const TSLanguage *tree_sitter_cmake(void);
const TSLanguage *tree_sitter_html(void);
const TSLanguage *tree_sitter_css(void);
const TSLanguage *tree_sitter_javascript(void);
const TSLanguage *tree_sitter_typescript(void);
const TSLanguage *tree_sitter_tsx(void);
const TSLanguage *tree_sitter_json(void);
const TSLanguage *tree_sitter_markdown(void);
const TSLanguage *tree_sitter_python(void);
const TSLanguage *tree_sitter_bash(void);
const TSLanguage *tree_sitter_sql(void);
const TSLanguage *tree_sitter_yaml(void);
}

namespace {

LanguageDefinition makeDef(LanguageId id, const QString &name, const QStringList &exts,
                           const QStringList &filenames, const TSLanguage *(*tsLang)(),
                           const QString &queryDir, const CommentTokens &comments,
                           const QString &brackets, const QString &indentTriggers,
                           const QString &languageServer)
{
    LanguageDefinition def;
    def.id = id;
    def.displayName = name;
    def.extensions = exts;
    def.filenames = filenames;
    def.treeSitterLanguage = tsLang;
    if (!queryDir.isEmpty()) {
        def.highlightQueryResourcePath =
            QStringLiteral(":/editerako/syntax/") + queryDir + QStringLiteral("/highlights.scm");
    }
    def.commentTokens = comments;
    def.brackets = brackets;
    def.indentTriggers = indentTriggers;
    def.languageServer = languageServer;
    return def;
}

const QString kBrackets = QStringLiteral("()[]{}");
const QString kCLikeIndent = QStringLiteral("{([:");
const CommentTokens kCFamily{QStringLiteral("//"), QStringLiteral("/*"), QStringLiteral("*/")};
const CommentTokens kHash{QStringLiteral("#"), {}, {}};
const CommentTokens kHtml{{}, QStringLiteral("<!--"), QStringLiteral("-->")};

} // namespace

const QList<LanguageDefinition> &LanguageRegistry::all()
{
    static const QList<LanguageDefinition> kDefs = {
        makeDef(LanguageId::PlainText, QStringLiteral("Plain Text"), {}, {}, nullptr, {}, {}, {}, {},
                {}),
        makeDef(LanguageId::C, QStringLiteral("C"), {QStringLiteral("c")}, {}, tree_sitter_c,
                QStringLiteral("c"), kCFamily, kBrackets, kCLikeIndent, QStringLiteral("clangd")),
        makeDef(LanguageId::Cpp, QStringLiteral("C++"),
                {QStringLiteral("cpp"), QStringLiteral("cc"), QStringLiteral("cxx"),
                 QStringLiteral("c++"), QStringLiteral("h"), QStringLiteral("hpp"),
                 QStringLiteral("hh"), QStringLiteral("hxx")},
                {}, tree_sitter_cpp, QStringLiteral("cpp"), kCFamily, kBrackets, kCLikeIndent,
                QStringLiteral("clangd")),
        makeDef(LanguageId::CMake, QStringLiteral("CMake"), {QStringLiteral("cmake")},
                {QStringLiteral("CMakeLists.txt")}, tree_sitter_cmake, QStringLiteral("cmake"),
                kHash, kBrackets, kCLikeIndent, {}),
        makeDef(LanguageId::Html, QStringLiteral("HTML"),
                {QStringLiteral("html"), QStringLiteral("htm")}, {}, tree_sitter_html,
                QStringLiteral("html"), kHtml, kBrackets, QStringLiteral("<"), {}),
        makeDef(LanguageId::Css, QStringLiteral("CSS"), {QStringLiteral("css")}, {}, tree_sitter_css,
                QStringLiteral("css"), {{}, QStringLiteral("/*"), QStringLiteral("*/")}, kBrackets,
                kCLikeIndent, {}),
        makeDef(LanguageId::JavaScript, QStringLiteral("JavaScript"),
                {QStringLiteral("js"), QStringLiteral("mjs"), QStringLiteral("cjs")}, {},
                tree_sitter_javascript, QStringLiteral("javascript"), kCFamily, kBrackets,
                kCLikeIndent, {}),
        makeDef(LanguageId::TypeScript, QStringLiteral("TypeScript"), {QStringLiteral("ts")}, {},
                tree_sitter_typescript, QStringLiteral("typescript"), kCFamily, kBrackets,
                kCLikeIndent, {}),
        makeDef(LanguageId::Tsx, QStringLiteral("TSX"), {QStringLiteral("tsx")}, {}, tree_sitter_tsx,
                QStringLiteral("tsx"), kCFamily, kBrackets, kCLikeIndent, {}),
        makeDef(LanguageId::Json, QStringLiteral("JSON"), {QStringLiteral("json")}, {},
                tree_sitter_json, QStringLiteral("json"), {}, kBrackets, kCLikeIndent, {}),
        makeDef(LanguageId::Markdown, QStringLiteral("Markdown"),
                {QStringLiteral("md"), QStringLiteral("markdown")}, {}, tree_sitter_markdown,
                QStringLiteral("markdown"), kHtml, kBrackets, {}, {}),
        makeDef(LanguageId::Python, QStringLiteral("Python"), {QStringLiteral("py")}, {},
                tree_sitter_python, QStringLiteral("python"), kHash, kBrackets,
                QStringLiteral(":"), {}),
        makeDef(LanguageId::Shell, QStringLiteral("Shell"),
                {QStringLiteral("sh"), QStringLiteral("bash"), QStringLiteral("zsh")}, {},
                tree_sitter_bash, QStringLiteral("bash"), kHash, kBrackets, kCLikeIndent, {}),
        makeDef(LanguageId::Sql, QStringLiteral("SQL"), {QStringLiteral("sql")}, {}, tree_sitter_sql,
                QStringLiteral("sql"),
                {QStringLiteral("--"), QStringLiteral("/*"), QStringLiteral("*/")}, kBrackets,
                kCLikeIndent, {}),
        makeDef(LanguageId::Yaml, QStringLiteral("YAML"),
                {QStringLiteral("yml"), QStringLiteral("yaml")}, {}, tree_sitter_yaml,
                QStringLiteral("yaml"), kHash, kBrackets, QStringLiteral(":"), {}),
    };
    return kDefs;
}

const LanguageDefinition &LanguageRegistry::definition(LanguageId id)
{
    for (const LanguageDefinition &def : all()) {
        if (def.id == id) {
            return def;
        }
    }
    return all().front();
}

LanguageId LanguageRegistry::idForPath(const QString &path)
{
    if (path.isEmpty()) {
        return LanguageId::PlainText;
    }

    const QFileInfo info(path);
    const QString name = info.fileName();
    for (const LanguageDefinition &def : all()) {
        for (const QString &filename : def.filenames) {
            if (name.compare(filename, Qt::CaseInsensitive) == 0) {
                return def.id;
            }
        }
    }
    return idForExtension(info.suffix());
}

LanguageId LanguageRegistry::idForExtension(const QString &extension)
{
    const QString ext = extension.toLower();
    if (ext.isEmpty()) {
        return LanguageId::PlainText;
    }
    for (const LanguageDefinition &def : all()) {
        if (def.extensions.contains(ext)) {
            return def.id;
        }
    }
    return LanguageId::PlainText;
}

QString LanguageRegistry::displayName(LanguageId id)
{
    return definition(id).displayName;
}

QString LanguageRegistry::languageIdString(LanguageId id)
{
    switch (id) {
    case LanguageId::C:
        return QStringLiteral("c");
    case LanguageId::Cpp:
        return QStringLiteral("cpp");
    case LanguageId::CMake:
        return QStringLiteral("cmake");
    case LanguageId::Html:
        return QStringLiteral("html");
    case LanguageId::Css:
        return QStringLiteral("css");
    case LanguageId::JavaScript:
        return QStringLiteral("javascript");
    case LanguageId::TypeScript:
        return QStringLiteral("typescript");
    case LanguageId::Tsx:
        return QStringLiteral("typescriptreact");
    case LanguageId::Json:
        return QStringLiteral("json");
    case LanguageId::Markdown:
        return QStringLiteral("markdown");
    case LanguageId::Python:
        return QStringLiteral("python");
    case LanguageId::Shell:
        return QStringLiteral("shellscript");
    case LanguageId::Sql:
        return QStringLiteral("sql");
    case LanguageId::Yaml:
        return QStringLiteral("yaml");
    case LanguageId::PlainText:
        break;
    }
    return {};
}

const TSLanguage *LanguageRegistry::tsLanguage(LanguageId id)
{
    const auto fn = definition(id).treeSitterLanguage;
    return fn ? fn() : nullptr;
}

QString LanguageRegistry::highlightQueryResourcePath(LanguageId id)
{
    return definition(id).highlightQueryResourcePath;
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
    return definition(id).commentTokens;
}
