#ifndef EDITERAKO_SVGVIEWER_H
#define EDITERAKO_SVGVIEWER_H

#include <QString>
#include <QWidget>

class QSvgWidget;

class SvgViewer : public QWidget
{
public:
    explicit SvgViewer(QWidget *parent = nullptr);

    bool load(const QString &filePath);
    [[nodiscard]] QString filePath() const { return m_filePath; }

private:
    QString m_filePath;
    QSvgWidget *m_svg = nullptr;
};

#endif
