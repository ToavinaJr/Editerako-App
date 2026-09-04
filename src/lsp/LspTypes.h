#ifndef EDITERAKO_LSPTYPES_H
#define EDITERAKO_LSPTYPES_H

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>
#include <QVector>

struct LspPosition {
    int line = 0;
    int character = 0;
};

struct LspRange {
    LspPosition start;
    LspPosition end;
};

struct LspLocation {
    QString uri;
    LspRange range;
};

enum class LspSeverity {
    Error = 1,
    Warning = 2,
    Information = 3,
    Hint = 4,
};

struct LspDiagnostic {
    LspRange range;
    LspSeverity severity = LspSeverity::Error;
    QString message;
    QString source;
    QString code;
};

struct LspCompletionItem {
    QString label;
    QString detail;
    QString documentation;
    QString insertText;
    QString sortText;
    QString filterText;
    int kind = 0;
    bool hasTextEdit = false;
    LspRange textEditRange;
};

struct LspTextEdit {
    QString uri;
    LspRange range;
    QString newText;
};

struct LspSignatureHelp {
    QString label;
    QString documentation;
    int activeParameter = 0;
    bool valid = false;
};

struct LspHover {
    QString contents;
    LspRange range;
    bool hasRange = false;
};

struct LspSymbol {
    QString name;
    int kind = 0;
    LspRange range;
    LspRange selectionRange;
    QString uri;
};

[[nodiscard]] QJsonObject lspPositionToJson(const LspPosition &pos);
[[nodiscard]] LspPosition lspPositionFromJson(const QJsonObject &obj);
[[nodiscard]] QJsonObject lspRangeToJson(const LspRange &range);
[[nodiscard]] LspRange lspRangeFromJson(const QJsonObject &obj);
[[nodiscard]] LspLocation lspLocationFromJson(const QJsonObject &obj);
[[nodiscard]] LspDiagnostic lspDiagnosticFromJson(const QJsonObject &obj);
[[nodiscard]] LspCompletionItem lspCompletionItemFromJson(const QJsonObject &obj);
[[nodiscard]] LspHover lspHoverFromJson(const QJsonObject &obj);
[[nodiscard]] QVector<LspLocation> lspLocationsFromJson(const QJsonValue &value);
[[nodiscard]] QVector<LspCompletionItem> lspCompletionItemsFromJson(const QJsonValue &value);
[[nodiscard]] QVector<LspSymbol> lspDocumentSymbolsFromJson(const QJsonValue &value);
[[nodiscard]] QVector<LspSymbol> lspWorkspaceSymbolsFromJson(const QJsonValue &value);
[[nodiscard]] QString lspMarkupToString(const QJsonValue &value);
[[nodiscard]] LspSignatureHelp lspSignatureHelpFromJson(const QJsonValue &value);
[[nodiscard]] QVector<LspTextEdit> lspTextEditsFromWorkspaceEdit(const QJsonObject &edit);
[[nodiscard]] QString lspFileUri(const QString &path);
[[nodiscard]] QString lspUriToPath(const QString &uri);

#endif
