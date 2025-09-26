#include "syntaxhighlighter.h"
#include <QDebug>
#include <QRegularExpression>

// Importer Tree-sitter en C (évite le name mangling en C++)
extern "C" {
#include <tree_sitter/api.h>
TSLanguage *tree_sitter_cpp();
TSLanguage *tree_sitter_html();
}

// Liste des mots-clés C++ (pattern thread-safe pour éviter le warning non-POD static)
static const QStringList& cppKeywords() {
    static const QStringList keywords = {
        "alignas", "alignof", "and", "and_eq", "asm", "auto", "bitand", "bitor", "bool", "break",
        "case", "catch", "char", "char16_t", "char32_t", "class", "compl", "const", "constexpr",
        "const_cast", "continue", "decltype", "default", "delete", "do", "double", "dynamic_cast",
        "else", "enum", "explicit", "export", "extern", "false", "float", "for", "friend", "goto",
        "if", "inline", "int", "long", "mutable", "namespace", "new", "noexcept", "not", "not_eq", "nullptr",
        "operator", "or", "or_eq", "private", "protected", "public", "register", "reinterpret_cast", "return", "short",
        "signed", "sizeof", "static", "static_assert", "static_cast", "struct", "switch", "template", "this", "thread_local",
        "throw", "true", "try", "typedef", "typeid", "typename", "union", "unsigned", "using", "virtual", "void",
        "volatile", "wchar_t", "while", "xor", "xor_eq"
    };
    return keywords;
}

SyntaxHighlighter::SyntaxHighlighter(CodeEditor *editor, Language lang)
    : QSyntaxHighlighter(editor ? editor->document() : nullptr), language(lang), parser(nullptr), tree(nullptr)
{
    if (!editor) {
        qWarning() << "SyntaxHighlighter: editor is nullptr!";
        return;
    }

    parser = ts_parser_new();
    if (!parser) {
        qWarning() << "Tree-sitter parser could not be created!";
        return;
    }

    if (language == CPP) {
        if (!ts_parser_set_language(parser, tree_sitter_cpp())) {
            qWarning() << "Unable to set tree-sitter CPP language!";
        }
    } else {
        if (!ts_parser_set_language(parser, tree_sitter_html())) {
            qWarning() << "Unable to set tree-sitter HTML language!";
        }
    }

    setupFormats();
}

SyntaxHighlighter::~SyntaxHighlighter() {
    if (tree) {
        ts_tree_delete(tree);
        tree = nullptr;
    }
    if (parser) {
        ts_parser_delete(parser);
        parser = nullptr;
    }
}

void SyntaxHighlighter::setupFormats() {
    keywordFormat.setForeground(QColor(86, 156, 214));
    keywordFormat.setFontWeight(QFont::Bold);

    typeFormat.setForeground(QColor(78, 201, 176));
    typeFormat.setFontWeight(QFont::Bold);

    stringFormat.setForeground(QColor(214, 157, 133));

    commentFormat.setForeground(QColor(106, 153, 85));

    numberFormat.setForeground(QColor(181, 206, 168));

    preprocFormat.setForeground(QColor(197, 134, 192));
    preprocFormat.setFontItalic(true);

    functionFormat.setForeground(QColor(220, 220, 170));
    functionFormat.setFontItalic(true);

    variableFormat.setForeground(QColor(156, 220, 254));

    parameterFormat.setForeground(QColor(215, 186, 125));

    punctuationFormat.setForeground(QColor(212, 212, 212));

    operatorFormat.setForeground(QColor(181, 206, 168));

    namespaceFormat.setForeground(QColor(255, 136, 0));
    namespaceFormat.setFontWeight(QFont::Bold);
}

void SyntaxHighlighter::highlightBlock(const QString &text) {
    if (language == CPP)
        highlightCpp(text);
    else
        highlightHtml(text);
}

void SyntaxHighlighter::highlightCpp(const QString &text) {
    if (!parser) return;

    QByteArray ba = text.toUtf8();
    const char *code = ba.constData();

    if (tree) ts_tree_delete(tree);
    tree = ts_parser_parse_string(parser, nullptr, code, static_cast<uint32_t>(ba.size()));
    if (!tree) return;

    TSNode root = ts_tree_root_node(tree);

    std::function<void(TSNode)> visit = [&](TSNode node) {
        const char *type = ts_node_type(node);
        int start = static_cast<int>(ts_node_start_byte(node));
        int end = static_cast<int>(ts_node_end_byte(node));
        int len = end - start;
        if (len <= 0) return;

        QString qtype = QString::fromUtf8(type);

        // Directives préprocesseur
        if (qtype.startsWith("preproc")) {
            setFormat(start, len, preprocFormat);
        } else if (qtype == "comment") {
            setFormat(start, len, commentFormat);
        } else if (qtype == "string_literal") {
            setFormat(start, len, stringFormat);
        } else if (qtype == "number_literal") {
            setFormat(start, len, numberFormat);
        } else if (qtype == "primitive_type" || qtype == "type_identifier") {
            setFormat(start, len, typeFormat);
        } else if (qtype == "function_definition" || qtype == "function_declarator" ||
                   qtype == "operator_cast" || qtype == "operator_cast_definition" ||
                   qtype == "function" || qtype == "function_call") {
            setFormat(start, len, functionFormat);
        } else if (qtype == "identifier") {
            setFormat(start, len, variableFormat);
        } else if (qtype == "parameter_declaration") {
            setFormat(start, len, parameterFormat);
        } else if (qtype == "namespace" || qtype == "namespace_definition") {
            setFormat(start, len, namespaceFormat);
        } else if (qtype == "class_specifier" || qtype == "struct_specifier") {
            setFormat(start, len, keywordFormat);
        } else if (qtype == "operator_name") {
            setFormat(start, len, operatorFormat);
        } else if (
            qtype == "{" || qtype == "}" || qtype == "(" || qtype == ")" ||
            qtype == "[" || qtype == "]" || qtype == ";" || qtype == "," ) {
            setFormat(start, len, punctuationFormat);
        }

        // Coloration du nom du namespace ou de la classe
        if (qtype == "namespace" || qtype == "namespace_definition" ||
            qtype == "class_specifier" || qtype == "struct_specifier") {
            uint32_t n = ts_node_child_count(node);
            for (uint32_t i = 0; i < n; ++i) {
                TSNode child = ts_node_child(node, i);
                QString childType = QString::fromUtf8(ts_node_type(child));
                if (childType == "identifier") {
                    setFormat(
                        static_cast<int>(ts_node_start_byte(child)),
                        static_cast<int>(ts_node_end_byte(child) - ts_node_start_byte(child)),
                        namespaceFormat
                        );
                }
            }
        }

        // Recurse on children
        uint32_t n = ts_node_child_count(node);
        for (uint32_t i = 0; i < n; ++i) {
            visit(ts_node_child(node, i));
        }
    };

    visit(root);

    // Coloration explicite des mots-clés (if, return, public, ...)
    QRegularExpression wordRegex("\\b([a-zA-Z_][a-zA-Z0-9_]*)\\b");
    auto it = wordRegex.globalMatch(text);
    while (it.hasNext()) {
        auto match = it.next();
        QString word = match.captured(1);
        if (cppKeywords().contains(word)) {
            setFormat(match.capturedStart(1), match.capturedLength(1), keywordFormat);
        }
    }
}

void SyntaxHighlighter::highlightHtml(const QString &text) {
    if (!parser) return;

    QByteArray ba = text.toUtf8();
    const char *code = ba.constData();

    if (tree) ts_tree_delete(tree);
    tree = ts_parser_parse_string(parser, nullptr, code, static_cast<uint32_t>(ba.size()));
    if (!tree) return;

    TSNode root = ts_tree_root_node(tree);

    std::function<void(TSNode)> visit = [&](TSNode node) {
        const char *type = ts_node_type(node);
        int start = static_cast<int>(ts_node_start_byte(node));
        int end = static_cast<int>(ts_node_end_byte(node));
        int len = end - start;
        if (len <= 0) return;

        QString qtype = QString::fromUtf8(type);
        if (qtype == "tag_name")
            setFormat(start, len, keywordFormat);
        else if (qtype == "attribute_name")
            setFormat(start, len, typeFormat);
        else if (qtype == "string")
            setFormat(start, len, stringFormat);
        else if (qtype == "comment")
            setFormat(start, len, commentFormat);

        // Recurse on children
        uint32_t n = ts_node_child_count(node);
        for (uint32_t i = 0; i < n; ++i) {
            visit(ts_node_child(node, i));
        }
    };

    visit(root);
}
