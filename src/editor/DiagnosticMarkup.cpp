#include "editor/DiagnosticMarkup.h"

#include <QPlainTextEdit>
#include <QTextBlock>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextDocument>
#include <QtGlobal>

int documentPositionAt(const QTextDocument *document, int line, int character)
{
    if (!document) {
        return 0;
    }
    QTextBlock block = document->findBlockByNumber(line);
    if (!block.isValid()) {
        return qMax(0, document->characterCount() - 1);
    }
    const int maxOffset = qMax(0, block.length() - 1);
    return block.position() + qBound(0, character, maxOffset);
}

QColor diagnosticColor(EditorDiagnostic::Severity severity)
{
    switch (severity) {
    case EditorDiagnostic::Severity::Error:
        return QColor(232, 80, 80);
    case EditorDiagnostic::Severity::Warning:
        return QColor(220, 180, 50);
    case EditorDiagnostic::Severity::Information:
        return QColor(80, 160, 230);
    case EditorDiagnostic::Severity::Hint:
        return QColor(140, 140, 140);
    }
    return QColor(232, 80, 80);
}

QList<QTextEdit::ExtraSelection>
diagnosticExtraSelections(QPlainTextEdit *editor, const QVector<EditorDiagnostic> &diagnostics)
{
    QList<QTextEdit::ExtraSelection> extras;
    if (!editor) {
        return extras;
    }
    QTextDocument *doc = editor->document();
    for (const EditorDiagnostic &diag : diagnostics) {
        QTextEdit::ExtraSelection sel;
        sel.cursor = QTextCursor(doc);
        const int start = documentPositionAt(doc, diag.startLine, diag.startCharacter);
        int end = documentPositionAt(doc, diag.endLine, diag.endCharacter);
        if (end <= start) {
            end = start + 1;
        }
        sel.cursor.setPosition(start);
        sel.cursor.setPosition(end, QTextCursor::KeepAnchor);
        sel.format.setUnderlineStyle(QTextCharFormat::WaveUnderline);
        sel.format.setUnderlineColor(diagnosticColor(diag.severity));
        sel.format.setToolTip(diag.message);
        extras.append(sel);
    }
    return extras;
}

int diagnosticGutterExtraWidth(const QVector<EditorDiagnostic> &diagnostics)
{
    return diagnostics.isEmpty() ? 0 : 10;
}

QHash<int, EditorDiagnostic::Severity>
worstDiagnosticByLine(const QVector<EditorDiagnostic> &diagnostics)
{
    QHash<int, EditorDiagnostic::Severity> worst;
    for (const EditorDiagnostic &diag : diagnostics) {
        const auto it = worst.constFind(diag.startLine);
        if (it == worst.cend() || static_cast<int>(diag.severity) < static_cast<int>(it.value())) {
            worst.insert(diag.startLine, diag.severity);
        }
    }
    return worst;
}
