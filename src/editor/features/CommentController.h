#ifndef EDITERAKO_COMMENTCONTROLLER_H
#define EDITERAKO_COMMENTCONTROLLER_H

#include <QString>

class QPlainTextEdit;

class CommentController
{
public:
    explicit CommentController(QPlainTextEdit *editor);

    void toggleLineComment(const QString &lineMarker);
    void toggleBlockComment(const QString &open, const QString &close);

private:
    QPlainTextEdit *m_editor = nullptr;
};

#endif
