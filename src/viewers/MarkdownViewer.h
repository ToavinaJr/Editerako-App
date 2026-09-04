#ifndef EDITERAKO_MARKDOWNVIEWER_H
#define EDITERAKO_MARKDOWNVIEWER_H

#include <QString>
#include <QWidget>

class QTextBrowser;

class MarkdownViewer : public QWidget
{
    Q_OBJECT

public:
    explicit MarkdownViewer(QWidget *parent = nullptr);

    bool load(const QString &filePath);
    [[nodiscard]] QString filePath() const { return m_filePath; }

private:
    QString m_filePath;
    QTextBrowser *m_browser = nullptr;
};

#endif
