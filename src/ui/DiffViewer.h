#ifndef EDITERAKO_DIFFVIEWER_H
#define EDITERAKO_DIFFVIEWER_H

#include <QWidget>

class QLabel;
class QPlainTextEdit;

class DiffViewer : public QWidget
{
    Q_OBJECT
public:
    explicit DiffViewer(QWidget *parent = nullptr);
    void setDiff(const QString &path, const QString &text);
    void setTexts(const QString &title, const QString &left, const QString &right,
                  const QString &leftName = QStringLiteral("disk"),
                  const QString &rightName = QStringLiteral("editor"));

private:
    QLabel *m_title = nullptr;
    QPlainTextEdit *m_text = nullptr;
};

#endif

