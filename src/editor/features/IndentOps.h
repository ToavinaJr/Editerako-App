#ifndef EDITERAKO_INDENTOPS_H
#define EDITERAKO_INDENTOPS_H

#include <QString>
#include <QStringList>

[[nodiscard]] QString indentUnit(int tabSize, bool insertSpaces);
[[nodiscard]] QString leadingWhitespace(const QString &line);
void indentLines(QStringList *lines, int tabSize, bool insertSpaces);
void outdentLines(QStringList *lines, int tabSize, bool insertSpaces);
[[nodiscard]] QString smartIndentPrefix(const QString &currentLine, int tabSize, bool insertSpaces);
[[nodiscard]] QString convertIndentation(const QString &text, bool toSpaces, int tabSize);
[[nodiscard]] QString trimTrailingWhitespace(const QString &text);
[[nodiscard]] QString sortLinesText(const QString &text);

#endif
