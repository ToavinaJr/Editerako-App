#include "viewers/ImageViewer.h"

#include <QColor>
#include <QPaintEvent>
#include <QPainter>
#include <QPoint>
#include <QResizeEvent>
#include <QSizePolicy>
#include <QtGlobal>

ImageViewer::ImageViewer(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_OpaquePaintEvent);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

bool ImageViewer::load(const QString &filePath)
{
    QPixmap pixmap(filePath);
    if (pixmap.isNull()) {
        return false;
    }

    m_filePath = filePath;
    m_original = pixmap;
    updateScaledPixmap();
    return true;
}

void ImageViewer::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateScaledPixmap();
}

void ImageViewer::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.fillRect(rect(), QColor(0x1e, 0x1e, 0x1e));
    if (m_scaled.isNull()) {
        return;
    }

    const QSizeF logical = m_scaled.deviceIndependentSize();
    const int x = qRound((width() - logical.width()) / 2.0);
    const int y = qRound((height() - logical.height()) / 2.0);
    painter.drawPixmap(QPoint(x, y), m_scaled);
}

void ImageViewer::updateScaledPixmap()
{
    if (m_original.isNull() || width() < 1 || height() < 1) {
        m_scaled = {};
        return;
    }

    const qreal dpr = devicePixelRatioF();
    const QSize target(qMax(1, qRound(width() * dpr)),
                       qMax(1, qRound(height() * dpr)));
    m_scaled = m_original.scaled(target, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    m_scaled.setDevicePixelRatio(dpr);
    update();
}
