#include "lsp/LspPickerItems.h"

#include <QFileInfo>

QVector<LspPickerRow> lspLocationRows(const QVector<LspLocation> &locations)
{
    QVector<LspPickerRow> rows;
    rows.reserve(locations.size());
    int index = 0;
    for (const LspLocation &loc : locations) {
        LspPickerRow row;
        row.id = QString::number(index++);
        row.path = lspUriToPath(loc.uri);
        row.display = QFileInfo(row.path).fileName();
        row.hint = QStringLiteral("%1:%2").arg(row.display).arg(loc.range.start.line + 1);
        row.filterText = row.path;
        row.line = loc.range.start.line;
        row.character = loc.range.start.character;
        rows.append(row);
    }
    return rows;
}

QVector<LspPickerRow> lspSymbolRows(const QVector<LspSymbol> &symbols, const QString &fallbackPath)
{
    QVector<LspPickerRow> rows;
    rows.reserve(symbols.size());
    int index = 0;
    for (const LspSymbol &sym : symbols) {
        LspPickerRow row;
        row.id = QString::number(index++);
        row.path = lspUriToPath(sym.uri);
        if (row.path.isEmpty()) {
            row.path = fallbackPath;
        }
        row.display = sym.name;
        row.hint = QFileInfo(row.path).fileName() + QLatin1Char(':')
            + QString::number(sym.selectionRange.start.line + 1);
        row.filterText = sym.name + QLatin1Char(' ') + row.path;
        row.line = sym.selectionRange.start.line;
        row.character = sym.selectionRange.start.character;
        rows.append(row);
    }
    return rows;
}
