#ifndef EDITERAKO_OUTPUTPANEL_H
#define EDITERAKO_OUTPUTPANEL_H

#include <QWidget>

class QPlainTextEdit;

class OutputPanel : public QWidget
{
    Q_OBJECT

public:
    explicit OutputPanel(QWidget *parent = nullptr);

    void clear();
    void append(const QString &text);

private:
    QPlainTextEdit *m_text = nullptr;
};

#endif
