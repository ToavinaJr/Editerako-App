#ifndef EDITERAKO_DIAGNOSTICMARKUP_H
#define EDITERAKO_DIAGNOSTICMARKUP_H

#include "editor/EditorDiagnostic.h"

#include <QColor>
#include <QHash>
#include <QList>
#include <QTextEdit>
#include <QVector>

class QPlainTextEdit;

[[nodiscard]] QList<QTextEdit::ExtraSelection>
diagnosticExtraSelections(QPlainTextEdit *editor, const QVector<EditorDiagnostic> &diagnostics);

[[nodiscard]] QHash<int, EditorDiagnostic::Severity>
worstDiagnosticByLine(const QVector<EditorDiagnostic> &diagnostics);

[[nodiscard]] QColor diagnosticColor(EditorDiagnostic::Severity severity);

[[nodiscard]] int diagnosticGutterExtraWidth(const QVector<EditorDiagnostic> &diagnostics);

#endif
