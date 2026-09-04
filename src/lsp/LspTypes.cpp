#include "lsp/LspTypes.h"

#include <QFileInfo>
#include <QJsonArray>
#include <QStringList>
#include <QUrl>
#include <QtGlobal>

QJsonObject lspPositionToJson(const LspPosition &pos)
{
    return QJsonObject{{QStringLiteral("line"), pos.line},
                       {QStringLiteral("character"), pos.character}};
}

LspPosition lspPositionFromJson(const QJsonObject &obj)
{
    LspPosition pos;
    pos.line = obj.value(QStringLiteral("line")).toInt();
    pos.character = obj.value(QStringLiteral("character")).toInt();
    return pos;
}

QJsonObject lspRangeToJson(const LspRange &range)
{
    return QJsonObject{{QStringLiteral("start"), lspPositionToJson(range.start)},
                       {QStringLiteral("end"), lspPositionToJson(range.end)}};
}

LspRange lspRangeFromJson(const QJsonObject &obj)
{
    LspRange range;
    range.start = lspPositionFromJson(obj.value(QStringLiteral("start")).toObject());
    range.end = lspPositionFromJson(obj.value(QStringLiteral("end")).toObject());
    return range;
}

LspLocation lspLocationFromJson(const QJsonObject &obj)
{
    LspLocation loc;
    loc.uri = obj.value(QStringLiteral("uri")).toString();
    if (obj.contains(QStringLiteral("range"))) {
        loc.range = lspRangeFromJson(obj.value(QStringLiteral("range")).toObject());
    } else if (obj.contains(QStringLiteral("targetRange"))) {
        loc.uri = obj.value(QStringLiteral("targetUri")).toString();
        loc.range = lspRangeFromJson(obj.value(QStringLiteral("targetSelectionRange")).toObject());
        if (loc.range.start.line == 0 && loc.range.end.line == 0
            && loc.range.start.character == 0 && loc.range.end.character == 0) {
            loc.range = lspRangeFromJson(obj.value(QStringLiteral("targetRange")).toObject());
        }
    }
    return loc;
}

LspDiagnostic lspDiagnosticFromJson(const QJsonObject &obj)
{
    LspDiagnostic diag;
    diag.range = lspRangeFromJson(obj.value(QStringLiteral("range")).toObject());
    const int severity = obj.value(QStringLiteral("severity")).toInt(1);
    diag.severity = static_cast<LspSeverity>(qBound(1, severity, 4));
    diag.message = obj.value(QStringLiteral("message")).toString();
    diag.source = obj.value(QStringLiteral("source")).toString();
    const QJsonValue code = obj.value(QStringLiteral("code"));
    if (code.isString()) {
        diag.code = code.toString();
    } else if (code.isDouble()) {
        diag.code = QString::number(code.toInt());
    }
    return diag;
}

QString lspMarkupToString(const QJsonValue &value)
{
    if (value.isString()) {
        return value.toString();
    }
    if (value.isObject()) {
        const QJsonObject obj = value.toObject();
        if (obj.contains(QStringLiteral("value"))) {
            return obj.value(QStringLiteral("value")).toString();
        }
        if (obj.contains(QStringLiteral("language"))) {
            return obj.value(QStringLiteral("value")).toString();
        }
    }
    if (value.isArray()) {
        QStringList parts;
        const QJsonArray arr = value.toArray();
        for (const QJsonValue &item : arr) {
            const QString part = lspMarkupToString(item);
            if (!part.isEmpty()) {
                parts.append(part);
            }
        }
        return parts.join(QLatin1Char('\n'));
    }
    return {};
}

LspCompletionItem lspCompletionItemFromJson(const QJsonObject &obj)
{
    LspCompletionItem item;
    item.label = obj.value(QStringLiteral("label")).toString();
    item.detail = obj.value(QStringLiteral("detail")).toString();
    item.documentation = lspMarkupToString(obj.value(QStringLiteral("documentation")));
    item.insertText = obj.value(QStringLiteral("insertText")).toString();
    if (item.insertText.isEmpty() && obj.contains(QStringLiteral("textEdit"))) {
        item.insertText =
            obj.value(QStringLiteral("textEdit")).toObject().value(QStringLiteral("newText")).toString();
    }
    item.sortText = obj.value(QStringLiteral("sortText")).toString();
    item.filterText = obj.value(QStringLiteral("filterText")).toString();
    item.kind = obj.value(QStringLiteral("kind")).toInt();
    if (obj.contains(QStringLiteral("textEdit"))) {
        const QJsonObject edit = obj.value(QStringLiteral("textEdit")).toObject();
        item.hasTextEdit = edit.contains(QStringLiteral("range"));
        if (item.hasTextEdit) {
            item.textEditRange = lspRangeFromJson(edit.value(QStringLiteral("range")).toObject());
        } else if (edit.contains(QStringLiteral("insert"))) {
            item.hasTextEdit = true;
            item.textEditRange = lspRangeFromJson(edit.value(QStringLiteral("insert")).toObject());
        }
    }
    return item;
}

LspHover lspHoverFromJson(const QJsonObject &obj)
{
    LspHover hover;
    hover.contents = lspMarkupToString(obj.value(QStringLiteral("contents")));
    if (obj.contains(QStringLiteral("range"))) {
        hover.range = lspRangeFromJson(obj.value(QStringLiteral("range")).toObject());
        hover.hasRange = true;
    }
    return hover;
}

QVector<LspLocation> lspLocationsFromJson(const QJsonValue &value)
{
    QVector<LspLocation> out;
    if (value.isNull() || value.isUndefined()) {
        return out;
    }
    if (value.isObject()) {
        const LspLocation loc = lspLocationFromJson(value.toObject());
        if (!loc.uri.isEmpty()) {
            out.append(loc);
        }
        return out;
    }
    if (value.isArray()) {
        const QJsonArray arr = value.toArray();
        for (const QJsonValue &item : arr) {
            const LspLocation loc = lspLocationFromJson(item.toObject());
            if (!loc.uri.isEmpty()) {
                out.append(loc);
            }
        }
    }
    return out;
}

QVector<LspCompletionItem> lspCompletionItemsFromJson(const QJsonValue &value)
{
    QJsonArray arr;
    if (value.isArray()) {
        arr = value.toArray();
    } else if (value.isObject()) {
        arr = value.toObject().value(QStringLiteral("items")).toArray();
    }
    QVector<LspCompletionItem> out;
    for (const QJsonValue &item : arr) {
        out.append(lspCompletionItemFromJson(item.toObject()));
    }
    return out;
}

namespace {

void collectDocumentSymbols(const QJsonArray &arr, const QString &uri, QVector<LspSymbol> *out)
{
    for (const QJsonValue &item : arr) {
        const QJsonObject obj = item.toObject();
        LspSymbol sym;
        sym.name = obj.value(QStringLiteral("name")).toString();
        sym.kind = obj.value(QStringLiteral("kind")).toInt();
        sym.uri = uri;
        if (obj.contains(QStringLiteral("range"))) {
            sym.range = lspRangeFromJson(obj.value(QStringLiteral("range")).toObject());
            if (obj.contains(QStringLiteral("selectionRange"))) {
                sym.selectionRange =
                    lspRangeFromJson(obj.value(QStringLiteral("selectionRange")).toObject());
            } else {
                sym.selectionRange = sym.range;
            }
        } else if (obj.contains(QStringLiteral("location"))) {
            const LspLocation loc = lspLocationFromJson(obj.value(QStringLiteral("location")).toObject());
            sym.uri = loc.uri;
            sym.range = loc.range;
            sym.selectionRange = loc.range;
        }
        out->append(sym);
        if (obj.contains(QStringLiteral("children"))) {
            collectDocumentSymbols(obj.value(QStringLiteral("children")).toArray(), sym.uri, out);
        }
    }
}

} // namespace

QVector<LspSymbol> lspDocumentSymbolsFromJson(const QJsonValue &value)
{
    QVector<LspSymbol> out;
    if (value.isArray()) {
        collectDocumentSymbols(value.toArray(), {}, &out);
    }
    return out;
}

QVector<LspSymbol> lspWorkspaceSymbolsFromJson(const QJsonValue &value)
{
    QVector<LspSymbol> out;
    if (!value.isArray()) {
        return out;
    }
    const QJsonArray arr = value.toArray();
    for (const QJsonValue &item : arr) {
        const QJsonObject obj = item.toObject();
        LspSymbol sym;
        sym.name = obj.value(QStringLiteral("name")).toString();
        sym.kind = obj.value(QStringLiteral("kind")).toInt();
        const LspLocation loc = lspLocationFromJson(obj.value(QStringLiteral("location")).toObject());
        sym.uri = loc.uri;
        sym.range = loc.range;
        sym.selectionRange = loc.range;
        out.append(sym);
    }
    return out;
}

QString lspFileUri(const QString &path)
{
    if (path.isEmpty()) {
        return {};
    }
    return QUrl::fromLocalFile(QFileInfo(path).absoluteFilePath()).toString(QUrl::FullyEncoded);
}

QString lspUriToPath(const QString &uri)
{
    const QUrl url(uri);
    if (url.isLocalFile()) {
        return url.toLocalFile();
    }
    return {};
}

LspSignatureHelp lspSignatureHelpFromJson(const QJsonValue &value)
{
    LspSignatureHelp help;
    if (!value.isObject()) {
        return help;
    }
    const QJsonObject obj = value.toObject();
    const QJsonArray signatures = obj.value(QStringLiteral("signatures")).toArray();
    if (signatures.isEmpty()) {
        return help;
    }
    const int active = qBound(0, obj.value(QStringLiteral("activeSignature")).toInt(0),
                              signatures.size() - 1);
    const QJsonObject signature = signatures.at(active).toObject();
    help.label = signature.value(QStringLiteral("label")).toString();
    help.documentation = lspMarkupToString(signature.value(QStringLiteral("documentation")));
    help.activeParameter = obj.value(QStringLiteral("activeParameter")).toInt(0);
    help.valid = !help.label.isEmpty();
    return help;
}

QVector<LspTextEdit> lspTextEditsFromWorkspaceEdit(const QJsonObject &edit)
{
    QVector<LspTextEdit> out;
    const QJsonObject changes = edit.value(QStringLiteral("changes")).toObject();
    for (auto it = changes.begin(); it != changes.end(); ++it) {
        const QString uri = it.key();
        const QJsonArray edits = it.value().toArray();
        for (const QJsonValue &item : edits) {
            const QJsonObject obj = item.toObject();
            LspTextEdit textEdit;
            textEdit.uri = uri;
            textEdit.range = lspRangeFromJson(obj.value(QStringLiteral("range")).toObject());
            textEdit.newText = obj.value(QStringLiteral("newText")).toString();
            out.append(textEdit);
        }
    }
    const QJsonArray documentChanges = edit.value(QStringLiteral("documentChanges")).toArray();
    for (const QJsonValue &change : documentChanges) {
        const QJsonObject obj = change.toObject();
        if (!obj.contains(QStringLiteral("edits"))) {
            continue;
        }
        const QString uri =
            obj.value(QStringLiteral("textDocument")).toObject().value(QStringLiteral("uri")).toString();
        const QJsonArray edits = obj.value(QStringLiteral("edits")).toArray();
        for (const QJsonValue &item : edits) {
            const QJsonObject editObj = item.toObject();
            LspTextEdit textEdit;
            textEdit.uri = uri;
            textEdit.range = lspRangeFromJson(editObj.value(QStringLiteral("range")).toObject());
            textEdit.newText = editObj.value(QStringLiteral("newText")).toString();
            out.append(textEdit);
        }
    }
    return out;
}
