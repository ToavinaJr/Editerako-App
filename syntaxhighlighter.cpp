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
    // Keyword format (C++ keywords like if, for, while, return, class, etc.)
    keywordFormat.setForeground(QColor("#569CD6")); // VS Code/Qt Creator blue
    keywordFormat.setFontWeight(QFont::Bold);

    // Type format (primitive and user-defined types)
    typeFormat.setForeground(QColor("#4EC9B0")); // Teal/cyan for types
    typeFormat.setFontWeight(QFont::Bold);

    // String format
    stringFormat.setForeground(QColor("#D69D85")); // Light brown for strings

    // Comment format
    commentFormat.setForeground(QColor("#6A9955")); // Green for comments
    commentFormat.setFontItalic(true);

    // Function format
    functionFormat.setForeground(QColor("#DCDCAA")); // Yellow for functions
    functionFormat.setFontWeight(QFont::Bold);

    // Variable format
    variableFormat.setForeground(QColor("#9CDCFE")); // Light blue for variables

    // Preprocessor format
    preprocessorFormat.setForeground(QColor("#C586C0")); // Purple for #include, #define, etc.
    preprocessorFormat.setFontWeight(QFont::Bold);

    // Operator format
    operatorFormat.setForeground(QColor("#D4D4D4")); // Light gray for operators

    // Punctuator format (braces, parentheses, brackets)
    punctuatorFormat.setForeground(QColor("#FFD700")); // Gold for punctuation

    // Number format
    numberFormat.setForeground(QColor("#B5CEA8")); // Light green for numbers

    // Class format
    classFormat.setForeground(QColor("#4EC9B0")); // Teal for class names
    classFormat.setFontWeight(QFont::Bold);

    // Namespace format
    namespaceFormat.setForeground(QColor("#9CDCFE")); // Light blue for namespaces
}

void SyntaxHighlighter::highlightBlock(const QString &text) {
    if (!parser)
        return;

    if (language == CPP)
        highlightCpp(text);
    else
        highlightHtml(text);
}

void SyntaxHighlighter::highlightNodeRecursive(const TSNode &node, const QString &text) {
    const char *type = ts_node_type(node);
    QString nodeType(type);

    // Get byte positions from tree-sitter
    uint32_t startByte = ts_node_start_byte(node);
    uint32_t endByte = ts_node_end_byte(node);
    int len = static_cast<int>(endByte - startByte);
    
    if (len <= 0) {
        // Continue with child nodes even if current node has no range
        uint32_t childCount = ts_node_child_count(node);
        for (uint32_t i = 0; i < childCount; ++i) {
            TSNode child = ts_node_child(node, i);
            highlightNodeRecursive(child, text);
        }
        return;
    }

    // Convert byte positions to character positions for Qt
    QByteArray textBytes = text.toUtf8();
    int startChar = QString::fromUtf8(textBytes.left(static_cast<int>(startByte))).length();
    int endChar = QString::fromUtf8(textBytes.left(static_cast<int>(endByte))).length();
    int charLen = endChar - startChar;

    if (charLen <= 0) {
        // Continue with child nodes
        uint32_t childCount = ts_node_child_count(node);
        for (uint32_t i = 0; i < childCount; ++i) {
            TSNode child = ts_node_child(node, i);
            highlightNodeRecursive(child, text);
        }
        return;
    }

    // Comprehensive mapping of C++ Tree-sitter node types to formats
    QTextCharFormat *format = nullptr;

    // Keywords (control flow, storage, etc.)
    if (nodeType == "if" || nodeType == "else" || nodeType == "for" || 
        nodeType == "while" || nodeType == "do" || nodeType == "switch" || 
        nodeType == "case" || nodeType == "default" || nodeType == "break" || 
        nodeType == "continue" || nodeType == "return" || nodeType == "goto" ||
        nodeType == "try" || nodeType == "catch" || nodeType == "throw" ||
        nodeType == "class" || nodeType == "struct" || nodeType == "union" ||
        nodeType == "enum" || nodeType == "namespace" || nodeType == "using" ||
        nodeType == "typedef" || nodeType == "typename" || nodeType == "template" ||
        nodeType == "public" || nodeType == "private" || nodeType == "protected" ||
        nodeType == "virtual" || nodeType == "static" || nodeType == "const" ||
        nodeType == "volatile" || nodeType == "mutable" || nodeType == "inline" ||
        nodeType == "extern" || nodeType == "constexpr" || nodeType == "decltype" ||
        nodeType == "auto" || nodeType == "register" || nodeType == "thread_local" ||
        nodeType == "new" || nodeType == "delete" || nodeType == "sizeof" ||
        nodeType == "alignof" || nodeType == "noexcept" || nodeType == "nullptr" ||
        nodeType == "operator" || nodeType == "this" || nodeType == "friend" ||
        nodeType == "explicit" || nodeType == "override" || nodeType == "final") {
        format = &keywordFormat;
    }
    // Primitive types
    else if (nodeType == "primitive_type" || nodeType == "type_identifier" ||
             nodeType == "sized_type_specifier" ||
             nodeType == "int" || nodeType == "char" || nodeType == "float" ||
             nodeType == "double" || nodeType == "bool" || nodeType == "void" ||
             nodeType == "long" || nodeType == "short" || nodeType == "signed" ||
             nodeType == "unsigned" || nodeType == "wchar_t" || nodeType == "char16_t" ||
             nodeType == "char32_t" || nodeType == "size_t") {
        format = &typeFormat;
    }
    // Strings and character literals
    else if (nodeType == "string_literal" || nodeType == "char_literal" ||
             nodeType == "raw_string_literal") {
        format = &stringFormat;
    }
    // Comments
    else if (nodeType == "comment" || nodeType == "line_comment" ||
             nodeType == "block_comment") {
        format = &commentFormat;
    }
    // Functions
    else if (nodeType == "function_declarator" || nodeType == "function_definition" ||
             nodeType == "call_expression" || nodeType == "function_identifier") {
        format = &functionFormat;
    }
    // Preprocessor directives
    else if (nodeType == "preproc_include" || nodeType == "preproc_def" ||
             nodeType == "preproc_ifdef" || nodeType == "preproc_ifndef" ||
             nodeType == "preproc_if" || nodeType == "preproc_else" ||
             nodeType == "preproc_elif" || nodeType == "preproc_endif" ||
             nodeType == "preproc_line" || nodeType == "preproc_error" ||
             nodeType == "preproc_warning" || nodeType == "preproc_pragma" ||
             nodeType == "#include" || nodeType == "#define" || nodeType == "#ifdef" ||
             nodeType == "#ifndef" || nodeType == "#if" || nodeType == "#else" ||
             nodeType == "#elif" || nodeType == "#endif") {
        format = &preprocessorFormat;
    }
    // Operators
    else if (nodeType == "=" || nodeType == "+" || nodeType == "-" || 
             nodeType == "*" || nodeType == "/" || nodeType == "%" ||
             nodeType == "++" || nodeType == "--" || nodeType == "+=" ||
             nodeType == "-=" || nodeType == "*=" || nodeType == "/=" ||
             nodeType == "%=" || nodeType == "==" || nodeType == "!=" ||
             nodeType == "<" || nodeType == ">" || nodeType == "<=" ||
             nodeType == ">=" || nodeType == "&&" || nodeType == "||" ||
             nodeType == "!" || nodeType == "&" || nodeType == "|" ||
             nodeType == "^" || nodeType == "~" || nodeType == "<<" ||
             nodeType == ">>" || nodeType == "<<=" || nodeType == ">>=" ||
             nodeType == "&=" || nodeType == "|=" || nodeType == "^=" ||
             nodeType == "->" || nodeType == "." || nodeType == "::" ||
             nodeType == "?" || nodeType == ":" || nodeType == "binary_expression" ||
             nodeType == "unary_expression" || nodeType == "assignment_expression" ||
             nodeType == "update_expression") {
        format = &operatorFormat;
    }
    // Punctuators
    else if (nodeType == "{" || nodeType == "}" || nodeType == "(" ||
             nodeType == ")" || nodeType == "[" || nodeType == "]" ||
             nodeType == ";" || nodeType == "," || nodeType == "compound_statement" ||
             nodeType == "parameter_list" || nodeType == "argument_list") {
        format = &punctuatorFormat;
    }
    // Numbers
    else if (nodeType == "number_literal" || nodeType == "integer_literal" ||
             nodeType == "float_literal" || nodeType == "hex_literal" ||
             nodeType == "octal_literal" || nodeType == "binary_literal") {
        format = &numberFormat;
    }
    // Class/struct names
    else if (nodeType == "class_specifier" || nodeType == "struct_specifier" ||
             nodeType == "type_identifier" || nodeType == "class_name") {
        format = &classFormat;
    }
    // Namespace
    else if (nodeType == "namespace_identifier" || nodeType == "namespace_definition") {
        format = &namespaceFormat;
    }
    // Variables and identifiers
    else if (nodeType == "identifier" || nodeType == "field_identifier") {
        format = &variableFormat;
    }

    // Apply formatting if we found a matching format
    if (format) {
        setFormat(startChar, charLen, *format);
    }

    // Recursively process child nodes
    uint32_t childCount = ts_node_child_count(node);
    for (uint32_t i = 0; i < childCount; ++i) {
        TSNode child = ts_node_child(node, i);
        highlightNodeRecursive(child, text);
    }
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
    
    // Use recursive highlighting for comprehensive coverage
    highlightNodeRecursive(root, text);
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
