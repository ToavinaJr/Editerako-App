#include "editor/HoverPopup.h"

#include <QTextBrowser>
#include <QVBoxLayout>

HoverPopup::HoverPopup(QWidget *parent)
    : QFrame(parent, Qt::ToolTip | Qt::FramelessWindowHint)
    , m_browser(new QTextBrowser(this))
{
    setObjectName(QStringLiteral("hoverPopup"));
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    m_browser->setOpenExternalLinks(false);
    m_browser->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_browser->setFrameShape(QFrame::NoFrame);
    layout->addWidget(m_browser);
    resize(480, 220);
    hide();
}

void HoverPopup::showMarkdown(const QString &markdown, const QPoint &globalPos)
{
    if (markdown.trimmed().isEmpty()) {
        hidePopup();
        return;
    }
    m_browser->setMarkdown(markdown);
    move(globalPos + QPoint(12, 16));
    show();
    raise();
}

void HoverPopup::hidePopup()
{
    hide();
}
