#include "editor/HoverPopup.h"

#include <QApplication>
#include <QEvent>
#include <QTextBrowser>
#include <QTextDocument>
#include <QVBoxLayout>

HoverPopup::HoverPopup(QWidget *parent)
    : QFrame(parent, Qt::Tool | Qt::FramelessWindowHint | Qt::WindowDoesNotAcceptFocus)
    , m_browser(new QTextBrowser(this))
{
    setObjectName(QStringLiteral("hoverPopup"));
    setAttribute(Qt::WA_ShowWithoutActivating);
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setFocusPolicy(Qt::NoFocus);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    m_browser->setOpenExternalLinks(false);
    m_browser->setFocusPolicy(Qt::NoFocus);
    m_browser->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_browser->setFrameShape(QFrame::NoFrame);
    layout->addWidget(m_browser);
    resize(420, 80);
    hide();
}

void HoverPopup::showMarkdown(const QString &markdown, const QPoint &globalPos)
{
    if (markdown.trimmed().isEmpty()) {
        hidePopup();
        return;
    }
    m_browser->setMarkdown(markdown);
    m_browser->document()->setTextWidth(400);
    const int height = qBound(48, int(m_browser->document()->size().height()) + 24, 240);
    resize(420, height);
    move(globalPos + QPoint(16, 12));
    installAppFilter();
    show();
    raise();
}

void HoverPopup::hidePopup()
{
    removeAppFilter();
    hide();
}

void HoverPopup::installAppFilter()
{
    if (m_appFilterInstalled || !qApp) {
        return;
    }
    qApp->installEventFilter(this);
    m_appFilterInstalled = true;
}

void HoverPopup::removeAppFilter()
{
    if (!m_appFilterInstalled || !qApp) {
        return;
    }
    qApp->removeEventFilter(this);
    m_appFilterInstalled = false;
}

bool HoverPopup::eventFilter(QObject *watched, QEvent *event)
{
    if (!isVisible() || !event) {
        return QFrame::eventFilter(watched, event);
    }
    switch (event->type()) {
    case QEvent::MouseButtonPress:
    case QEvent::Wheel:
    case QEvent::KeyPress:
        hidePopup();
        break;
    default:
        break;
    }
    return QFrame::eventFilter(watched, event);
}
