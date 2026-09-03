#ifndef EDITERAKO_CODEEDITOR_H
#define EDITERAKO_CODEEDITOR_H

#include "editor/features/LineMovementController.h"
#include "editor/features/MultiCursorController.h"

#include <QPlainTextEdit>

class LineNumberArea;

class CodeEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit CodeEditor(QWidget *parent = nullptr);

    void lineNumberAreaPaintEvent(QPaintEvent *event);
    [[nodiscard]] int lineNumberAreaWidth() const;
    void setLineNumbersVisible(bool visible);
    [[nodiscard]] bool isLineNumbersVisible() const;

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect &rect, int dy);

private:
    LineNumberArea *m_lineNumberArea = nullptr;
    bool m_lineNumbersVisible = true;
    MultiCursorController m_multiCursor;
    LineMovementController m_lineMovement;
};

#endif
