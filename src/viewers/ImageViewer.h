#ifndef EDITERAKO_IMAGEVIEWER_H
#define EDITERAKO_IMAGEVIEWER_H

#include <QPixmap>
#include <QString>
#include <QWidget>

class ImageViewer : public QWidget
{
public:
    explicit ImageViewer(QWidget *parent = nullptr);

    bool load(const QString &filePath);
    [[nodiscard]] QString filePath() const { return m_filePath; }

protected:
    void resizeEvent(QResizeEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    void updateScaledPixmap();

    QString m_filePath;
    QPixmap m_original;
    QPixmap m_scaled;
};

#endif
