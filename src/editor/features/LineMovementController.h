#ifndef EDITERAKO_LINEMOVEMENTCONTROLLER_H
#define EDITERAKO_LINEMOVEMENTCONTROLLER_H

class QPlainTextEdit;

class LineMovementController
{
public:
    explicit LineMovementController(QPlainTextEdit *editor);

    void moveUp();
    void moveDown();

private:
    QPlainTextEdit *m_editor = nullptr;
};

#endif
