#include "syntaxhighlighter.h"
#include <QDebug>

// Importer Tree-sitter en C (évite le name mangling en C++)
extern "C" {
#include <tree_sitter/api.h>
TSLanguage *tree_sitter_cpp();
TSLanguage *tree_sitter_html();
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
    keywordFormat.setForeground(QColor("#569CD6"));
    keywordFormat.setFontWeight(QFont::Bold);

    typeFormat.setForeground(QColor("#4EC9B0"));
    typeFormat.setFontWeight(QFont::Bold);

    stringFormat.setForeground(QColor("#D69D85"));
    commentFormat.setForeground(QColor("#6A9955"));
}

void SyntaxHighlighter::highlightBlock(const QString &text) {
    if (!parser)
        return;

    if (language == CPP)
        highlightCpp(text);
    else
        highlightHtml(text);
}

void SyntaxHighlighter::highlightCpp(const QString &text) {
    if (!parser)
        return;

    QByteArray ba = text.toUtf8();
    const char *code = ba.constData();

    if (tree) {
        ts_tree_delete(tree);
        tree = nullptr;
    }
    tree = ts_parser_parse_string(parser, nullptr, code, static_cast<uint32_t>(ba.size()));
    if (!tree)
        return;

    TSNode root = ts_tree_root_node(tree);
    uint32_t childCount = ts_node_child_count(root);
    for (uint32_t i = 0; i < childCount; ++i) {
        TSNode child = ts_node_child(root, i);
        const char *type = ts_node_type(child);

        int start = static_cast<int>(ts_node_start_byte(child));
        int end = static_cast<int>(ts_node_end_byte(child));
        int len = end - start;
        if (len <= 0)
            continue;

        if (QString(type) == "string_literal")
            setFormat(start, len, stringFormat);
        else if (QString(type) == "primitive_type")
            setFormat(start, len, typeFormat);
        else if (QString(type) == "function_definition")
            setFormat(start, len, keywordFormat);
        else if (QString(type) == "comment")
            setFormat(start, len, commentFormat);
    }
}

void SyntaxHighlighter::highlightHtml(const QString &text) {
    if (!parser)
        return;

    QByteArray ba = text.toUtf8();
    const char *code = ba.constData();

    if (tree) {
        ts_tree_delete(tree);
        tree = nullptr;
    }
    tree = ts_parser_parse_string(parser, nullptr, code, static_cast<uint32_t>(ba.size()));
    if (!tree)
        return;

    TSNode root = ts_tree_root_node(tree);
    uint32_t childCount = ts_node_child_count(root);
    for (uint32_t i = 0; i < childCount; ++i) {
        TSNode child = ts_node_child(root, i);
        const char *type = ts_node_type(child);

        int start = static_cast<int>(ts_node_start_byte(child));
        int end = static_cast<int>(ts_node_end_byte(child));
        int len = end - start;
        if (len <= 0)
            continue;

        if (QString(type) == "tag_name")
            setFormat(start, len, keywordFormat);
        else if (QString(type) == "attribute_name")
            setFormat(start, len, typeFormat);
        else if (QString(type) == "string")
            setFormat(start, len, stringFormat);
        else if (QString(type) == "comment")
            setFormat(start, len, commentFormat);
    }
}
