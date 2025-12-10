#ifndef CODEEDITOR_H
#define CODEEDITOR_H

#include <QPlainTextEdit>
#include <QTextDocument>
#include <QPainter>
#include <QTextBlock>
#include <QWidget>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QTextCursor>
#include <QList>

class LineNumberArea;

class CodeEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    CodeEditor(QWidget *parent = nullptr);

    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int lineNumberAreaWidth();
    void setLineNumbersVisible(bool visible);
    bool isLineNumbersVisible() const;

protected:
    // Multi-cursor support and keyboard handling
    void mousePressEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect &rect, int dy);

private:
    QWidget *lineNumberArea;
    bool lineNumbersVisible;
    // Additional cursors for multi-cursor editing (excluding the primary cursor())
    QList<QTextCursor> extraCursors;

    // Helpers for multi-cursor editing
    void normalizeExtraCursors();
    void insertTextAtCursors(const QString &text);
    void deleteAtCursors(bool backspace);
    void swapLineUp();
    void swapLineDown();
};

class LineNumberArea : public QWidget
{
public:
    LineNumberArea(CodeEditor *editor) : QWidget(editor), codeEditor(editor)
    {}

    QSize sizeHint() const override
    {
        return QSize(codeEditor->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        codeEditor->lineNumberAreaPaintEvent(event);
    }

private:
    CodeEditor *codeEditor;
};

#endif // CODEEDITOR_H
