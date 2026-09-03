#ifndef EDITERAKO_OCCURRENCECONTROLLER_H
#define EDITERAKO_OCCURRENCECONTROLLER_H

class MultiCursorController;
class QPlainTextEdit;

class OccurrenceController
{
public:
    OccurrenceController(QPlainTextEdit *editor, MultiCursorController *multi);

    void selectNext();
    void selectAll();

private:
    QPlainTextEdit *m_editor = nullptr;
    MultiCursorController *m_multi = nullptr;
};

#endif
