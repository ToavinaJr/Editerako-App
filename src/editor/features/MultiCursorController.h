#ifndef EDITERAKO_MULTICURSORCONTROLLER_H
#define EDITERAKO_MULTICURSORCONTROLLER_H

#include <QList>
#include <QString>
#include <QTextCursor>

class QKeyEvent;
class QMouseEvent;
class QPaintEvent;
class QPlainTextEdit;

class MultiCursorController
{
public:
    explicit MultiCursorController(QPlainTextEdit *editor);

    [[nodiscard]] bool isEmpty() const { return m_extraCursors.isEmpty(); }
    [[nodiscard]] int extraCount() const { return static_cast<int>(m_extraCursors.size()); }

    bool handleMousePress(QMouseEvent *event);
    bool handleKeyPress(QKeyEvent *event);
    void paint(QPaintEvent *event) const;

    void toggleAt(const QTextCursor &clicked);
    void insertText(const QString &text);
    void deleteAtCursors(bool backspace);

private:
    void normalize();
    void clear();

    QPlainTextEdit *m_editor = nullptr;
    QList<QTextCursor> m_extraCursors;
};

#endif
