#ifndef EDITERAKO_CODEEDITOR_H
#define EDITERAKO_CODEEDITOR_H

#include "editor/features/CommentController.h"
#include "editor/features/IndentController.h"
#include "editor/features/LineEditCommands.h"
#include "editor/features/LineMovementController.h"
#include "editor/features/MultiCursorController.h"
#include "editor/features/OccurrenceController.h"
#include "editor/EditorDiagnostic.h"

#include <QPlainTextEdit>
#include <QPoint>
#include <QVector>

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

    void indentSelection();
    void outdentSelection();
    void toggleLineComment();
    void toggleBlockComment();
    void duplicateLine();
    void deleteLine();
    void moveLineUp();
    void moveLineDown();
    void selectLine();
    void joinLines();
    void sortSelectedLines();
    void trimTrailingWhitespace();
    void convertIndentationToSpaces();
    void convertIndentationToTabs();
    void selectNextOccurrence();
    void selectAllOccurrences();

    void setDiagnostics(const QVector<EditorDiagnostic> &diagnostics);
    [[nodiscard]] const QVector<EditorDiagnostic> &diagnostics() const { return m_diagnostics; }

signals:
    void completionRequested();
    void signatureHelpRequested();
    void hoverRequested(int line, int character, const QPoint &globalPos);
    void hoverCanceled();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect &rect, int dy);
    void emitHover();

private:
    LineNumberArea *m_lineNumberArea = nullptr;
    bool m_lineNumbersVisible = true;
    MultiCursorController m_multiCursor;
    LineMovementController m_lineMovement;
    IndentController m_indent;
    CommentController m_comments;
    LineEditCommands m_lineEdits;
    OccurrenceController m_occurrences;
    QVector<EditorDiagnostic> m_diagnostics;
    class QTimer *m_hoverTimer = nullptr;
    QPoint m_hoverLocalPos;
};

#endif
