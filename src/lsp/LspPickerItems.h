#ifndef EDITERAKO_LSPPICKERITEMS_H
#define EDITERAKO_LSPPICKERITEMS_H

#include "lsp/LspTypes.h"

#include <QString>
#include <QVector>

struct LspPickerRow {
    QString id;
    QString display;
    QString hint;
    QString filterText;
    QString path;
    int line = 0;
    int character = 0;
};

[[nodiscard]] QVector<LspPickerRow> lspLocationRows(const QVector<LspLocation> &locations);
[[nodiscard]] QVector<LspPickerRow> lspSymbolRows(const QVector<LspSymbol> &symbols,
                                                  const QString &fallbackPath = {});

#endif
