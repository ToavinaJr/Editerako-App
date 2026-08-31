#include "syntax/HighlightQuery.h"

#include <QFile>
#include <QtTest>

#include <cstring>

extern "C" {
struct TSLanguage;
TSLanguage *tree_sitter_cpp();
TSLanguage *tree_sitter_html();
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

QTEST_GUILESS_MAIN(HighlightQueryTest)
#include "HighlightQueryTest.moc"
