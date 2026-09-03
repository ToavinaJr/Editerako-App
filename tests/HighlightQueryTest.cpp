#include "syntax/HighlightQuery.h"
#include "syntax/LanguageRegistry.h"

#include <QFile>
#include <QtTest>

#include <cstring>

Q_DECLARE_METATYPE(LanguageId)

extern "C" {
struct TSLanguage;
const TSLanguage *tree_sitter_cpp(void);
const TSLanguage *tree_sitter_html(void);
}

class HighlightQueryTest : public QObject
{
    Q_OBJECT

private:
    struct Parsed {
        TSParser *parser = nullptr;
        TSTree *tree = nullptr;
        QByteArray utf8;

        ~Parsed()
        {
            if (tree) {
                ts_tree_delete(tree);
            }
            if (parser) {
                ts_parser_delete(parser);
            }
        }
    };

    static Parsed parse(const TSLanguage *language, const QByteArray &source)
    {
        Parsed parsed;
        parsed.utf8 = source;
        parsed.parser = ts_parser_new();
        if (!parsed.parser || !ts_parser_set_language(parsed.parser, language)) {
            return parsed;
        }
        parsed.tree = ts_parser_parse_string(
            parsed.parser, nullptr, source.constData(), static_cast<uint32_t>(source.size()));
        return parsed;
    }

    static bool hasCapture(const QVector<HighlightQuery::Capture> &caps, const QString &name)
    {
        for (const HighlightQuery::Capture &cap : caps) {
            if (cap.name == name) {
                return true;
            }
        }
        return false;
    }

    static QByteArray readQueryFile(const QString &relative)
    {
        QFile file(QStringLiteral(EDITERAKO_QUERY_DIR) + QLatin1Char('/') + relative);
        if (!file.open(QIODevice::ReadOnly)) {
            return {};
        }
        return file.readAll();
    }

private slots:
    void invalidQuery();
    void capturesIdentifiers();
    void matchPredicate();
    void shippedCppQueryCompilesAndHighlights();
    void shippedHtmlQueryCompilesAndHighlights();
    void allShippedQueriesCompile();
    void shippedLanguageCaptures_data();
    void shippedLanguageCaptures();
};

void HighlightQueryTest::invalidQuery()
{
    HighlightQuery query(tree_sitter_cpp(), QByteArrayLiteral("(not_a_real_node) @x"));
    QVERIFY(!query.isValid());
    QVERIFY(!query.errorString().isEmpty());
}

void HighlightQueryTest::capturesIdentifiers()
{
    const QByteArray src = QByteArrayLiteral("int main() { return 0; }");
    Parsed parsed = parse(tree_sitter_cpp(), src);
    QVERIFY(parsed.tree);

    HighlightQuery query(tree_sitter_cpp(), QByteArrayLiteral("(identifier) @variable"));
    QVERIFY2(query.isValid(), qPrintable(query.errorString()));

    const auto caps = query.captures(ts_tree_root_node(parsed.tree), 0,
                                     static_cast<uint32_t>(src.size()), parsed.utf8);
    QVERIFY(hasCapture(caps, QStringLiteral("variable")));
}

void HighlightQueryTest::matchPredicate()
{
    const QByteArray src = QByteArrayLiteral("namespace Foo { int x; } namespace bar { int y; }");
    Parsed parsed = parse(tree_sitter_cpp(), src);
    QVERIFY(parsed.tree);

    HighlightQuery query(tree_sitter_cpp(), QByteArrayLiteral(
        "((namespace_identifier) @type (#match? @type \"^[A-Z]\"))"));
    QVERIFY2(query.isValid(), qPrintable(query.errorString()));

    const auto caps = query.captures(ts_tree_root_node(parsed.tree), 0,
                                     static_cast<uint32_t>(src.size()), parsed.utf8);
    QVERIFY(hasCapture(caps, QStringLiteral("type")));
    for (const HighlightQuery::Capture &cap : caps) {
        if (cap.name != QLatin1String("type")) {
            continue;
        }
        const QByteArray text = parsed.utf8.mid(static_cast<int>(cap.startByte),
                                                static_cast<int>(cap.endByte - cap.startByte));
        QCOMPARE(text, QByteArrayLiteral("Foo"));
    }
}

void HighlightQueryTest::shippedCppQueryCompilesAndHighlights()
{
    const QByteArray scm = readQueryFile(QStringLiteral("cpp/highlights.scm"));
    QVERIFY(!scm.isEmpty());

    HighlightQuery query(tree_sitter_cpp(), scm);
    QVERIFY2(query.isValid(), qPrintable(query.errorString()));

    const QByteArray src = QByteArrayLiteral(
        "int main() { /* c */ const char *s = \"hi\"; return 0; }\n");
    Parsed parsed = parse(tree_sitter_cpp(), src);
    QVERIFY(parsed.tree);

    const auto caps = query.captures(ts_tree_root_node(parsed.tree), 0,
                                     static_cast<uint32_t>(src.size()), parsed.utf8);
    QVERIFY(hasCapture(caps, QStringLiteral("keyword")));
    QVERIFY(hasCapture(caps, QStringLiteral("type")) || hasCapture(caps, QStringLiteral("keyword")));
    QVERIFY(hasCapture(caps, QStringLiteral("function")));
    QVERIFY(hasCapture(caps, QStringLiteral("comment")));
    QVERIFY(hasCapture(caps, QStringLiteral("string")));
    QVERIFY(hasCapture(caps, QStringLiteral("number")));
}

void HighlightQueryTest::shippedHtmlQueryCompilesAndHighlights()
{
    const QByteArray scm = readQueryFile(QStringLiteral("html/highlights.scm"));
    QVERIFY(!scm.isEmpty());

    HighlightQuery query(tree_sitter_html(), scm);
    QVERIFY2(query.isValid(), qPrintable(query.errorString()));

    const QByteArray src = QByteArrayLiteral("<div class=\"x\"><!-- hi --></div>");
    Parsed parsed = parse(tree_sitter_html(), src);
    QVERIFY(parsed.tree);

    const auto caps = query.captures(ts_tree_root_node(parsed.tree), 0,
                                     static_cast<uint32_t>(src.size()), parsed.utf8);
    QVERIFY(hasCapture(caps, QStringLiteral("tag")));
    QVERIFY(hasCapture(caps, QStringLiteral("attribute")));
    QVERIFY(hasCapture(caps, QStringLiteral("string")));
    QVERIFY(hasCapture(caps, QStringLiteral("comment")));
}

void HighlightQueryTest::allShippedQueriesCompile()
{
    const QString prefix = QStringLiteral(":/editerako/syntax/");
    for (const LanguageDefinition &def : LanguageRegistry::all()) {
        if (def.id == LanguageId::PlainText) {
            continue;
        }
        QVERIFY2(def.treeSitterLanguage != nullptr, qPrintable(def.displayName));
        QVERIFY2(def.highlightQueryResourcePath.startsWith(prefix), qPrintable(def.displayName));
        const QString rel = def.highlightQueryResourcePath.mid(prefix.size());
        const QByteArray scm = readQueryFile(rel);
        QVERIFY2(!scm.isEmpty(), qPrintable(def.displayName));
        HighlightQuery query(def.treeSitterLanguage(), scm);
        QVERIFY2(query.isValid(), qPrintable(def.displayName + QLatin1Char(' ') + query.errorString()));
    }
}

void HighlightQueryTest::shippedLanguageCaptures_data()
{
    QTest::addColumn<LanguageId>("language");
    QTest::addColumn<QByteArray>("source");
    QTest::addColumn<QString>("capture");

    QTest::newRow("c") << LanguageId::C << QByteArray("int main() { return 0; /* c */ }\n")
                       << QStringLiteral("comment");
    QTest::newRow("python") << LanguageId::Python << QByteArray("def f():\n    return 1  # hi\n")
                            << QStringLiteral("comment");
    QTest::newRow("javascript") << LanguageId::JavaScript
                                << QByteArray("const x = \"hi\"; // c\n") << QStringLiteral("string");
    QTest::newRow("typescript") << LanguageId::TypeScript
                                << QByteArray("const x: number = 1; // c\n") << QStringLiteral("comment");
    QTest::newRow("tsx") << LanguageId::Tsx << QByteArray("const n = <div/>;\n")
                         << QStringLiteral("tag");
    QTest::newRow("json") << LanguageId::Json << QByteArray("{\"a\": 1}\n")
                          << QStringLiteral("number");
    QTest::newRow("css") << LanguageId::Css << QByteArray("div { color: red; /* c */ }\n")
                         << QStringLiteral("comment");
    QTest::newRow("bash") << LanguageId::Shell << QByteArray("echo hello # c\n")
                          << QStringLiteral("comment");
    QTest::newRow("cmake") << LanguageId::CMake << QByteArray("project(Foo) # c\n")
                           << QStringLiteral("comment");
    QTest::newRow("yaml") << LanguageId::Yaml << QByteArray("a: 1 # c\n")
                          << QStringLiteral("comment");
    QTest::newRow("markdown") << LanguageId::Markdown << QByteArray("# Title\n")
                              << QStringLiteral("keyword");
    QTest::newRow("sql") << LanguageId::Sql << QByteArray("SELECT 1 FROM t; -- c\n")
                         << QStringLiteral("comment");
}

void HighlightQueryTest::shippedLanguageCaptures()
{
    QFETCH(LanguageId, language);
    QFETCH(QByteArray, source);
    QFETCH(QString, capture);

    const LanguageDefinition &def = LanguageRegistry::definition(language);
    const QString rel =
        def.highlightQueryResourcePath.mid(QStringLiteral(":/editerako/syntax/").size());
    HighlightQuery query(def.treeSitterLanguage(), readQueryFile(rel));
    QVERIFY2(query.isValid(), qPrintable(query.errorString()));

    Parsed parsed = parse(def.treeSitterLanguage(), source);
    QVERIFY(parsed.tree);
    const auto caps = query.captures(ts_tree_root_node(parsed.tree), 0,
                                     static_cast<uint32_t>(source.size()), parsed.utf8);
    QVERIFY2(hasCapture(caps, capture), qPrintable(def.displayName + QLatin1Char(' ') + capture));
}

QTEST_GUILESS_MAIN(HighlightQueryTest)
#include "HighlightQueryTest.moc"
