#ifndef EDITERAKO_EDITORDIAGNOSTIC_H
#define EDITERAKO_EDITORDIAGNOSTIC_H

#include <QString>
#include <QVector>

struct EditorDiagnostic {
    enum class Severity {
        Error,
        Warning,
        Information,
        Hint,
    };

    int startLine = 0;
    int startCharacter = 0;
    int endLine = 0;
    int endCharacter = 0;
    Severity severity = Severity::Error;
    QString message;
};

[[nodiscard]] int documentPositionAt(const class QTextDocument *document, int line, int character);

#endif
