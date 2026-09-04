#include "viewers/SvgViewer.h"

#include <QScrollArea>
#include <QSvgRenderer>
#include <QSvgWidget>
#include <QVBoxLayout>
#include <Qt>

SvgViewer::SvgViewer(QWidget *parent)
    : QWidget(parent)
    , m_svg(new QSvgWidget)
{
    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setAlignment(Qt::AlignCenter);
    scroll->setWidget(m_svg);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(scroll);
}

bool SvgViewer::load(const QString &filePath)
{
    m_svg->load(filePath);
    if (!m_svg->renderer() || !m_svg->renderer()->isValid()) {
        return false;
    }
    m_filePath = filePath;
    return true;
}
