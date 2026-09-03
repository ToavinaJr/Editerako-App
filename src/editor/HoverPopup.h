#ifndef EDITERAKO_HOVERPOPUP_H
#define EDITERAKO_HOVERPOPUP_H

#include <QFrame>
#include <QString>

class QTextBrowser;

class HoverPopup : public QFrame
{
    Q_OBJECT

public:
    explicit HoverPopup(QWidget *parent = nullptr);

    void showMarkdown(const QString &markdown, const QPoint &globalPos);
    void hidePopup();

private:
    QTextBrowser *m_browser = nullptr;
};

#endif
