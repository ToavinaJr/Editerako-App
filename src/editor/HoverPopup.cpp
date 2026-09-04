#include "editor/HoverPopup.h"

#include "lsp/LspHoverHtml.h"

#include <QApplication>
#include <QEvent>
#include <QPalette>
#include <QTextBrowser>
#include <QTextDocument>
#include <QUrl>
#include <QVBoxLayout>

namespace {

QString hoverStyleSheet(bool dark)
{
    if (dark) {
        return QStringLiteral(
            "body { color: #cccccc; font-family: 'Segoe UI', sans-serif; font-size: 12px; }"
            ".hover-title { color: #9cdcfe; font-weight: 600; margin-bottom: 4px; }"
            ".hk { color: #569cd6; } .ht { color: #4ec9b0; } .hf { color: #dcdcaa; }"
            ".hi { color: #d4d4d4; } .hp { color: #d4d4d4; } .hs { color: #ce9178; }"
            ".hc { color: #6a9955; } .hn { color: #b5cea8; }"
            "pre.hover-code, code.hover-code { font-family: Consolas, 'Courier New', monospace;"
            "  background: #1e1e1e; color: #d4d4d4; }"
            "pre.hover-code { padding: 6px 8px; border-radius: 4px; }"
            "code.hover-code { padding: 1px 4px; border-radius: 3px; }"
            "a.hover-def { color: #4fc1ff; text-decoration: none; }"
            "ul { margin: 4px 0 4px 16px; padding: 0; }");
    }
    return QStringLiteral(
        "body { color: #1e1e1e; font-family: 'Segoe UI', sans-serif; font-size: 12px; }"
        ".hover-title { color: #0451a5; font-weight: 600; margin-bottom: 4px; }"
        ".hk { color: #0000ff; } .ht { color: #267f99; } .hf { color: #795e26; }"
        ".hi { color: #001080; } .hp { color: #1e1e1e; } .hs { color: #a31515; }"
        ".hc { color: #008000; } .hn { color: #098658; }"
        "pre.hover-code, code.hover-code { font-family: Consolas, 'Courier New', monospace;"
        "  background: #f3f3f3; color: #1e1e1e; }"
        "pre.hover-code { padding: 6px 8px; border-radius: 4px; }"
        "code.hover-code { padding: 1px 4px; border-radius: 3px; }"
        "a.hover-def { color: #006ab1; text-decoration: none; }"
        "ul { margin: 4px 0 4px 16px; padding: 0; }");
}

} // namespace

HoverPopup::HoverPopup(QWidget *parent)
    : QFrame(parent, Qt::Tool | Qt::FramelessWindowHint | Qt::WindowDoesNotAcceptFocus)
    , m_browser(new QTextBrowser(this))
{
    setObjectName(QStringLiteral("hoverPopup"));
    setAttribute(Qt::WA_ShowWithoutActivating);
    setFocusPolicy(Qt::NoFocus);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    m_browser->setOpenExternalLinks(false);
    m_browser->setOpenLinks(false);
    m_browser->setFocusPolicy(Qt::NoFocus);
    m_browser->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_browser->setFrameShape(QFrame::NoFrame);
    layout->addWidget(m_browser);
    resize(460, 80);
    hide();

    connect(m_browser, &QTextBrowser::anchorClicked, this, [this](const QUrl &url) {
        if (url.scheme() == QLatin1String("editerako")) {
            emit goToDefinitionRequested();
            hidePopup();
        }
    });
}

void HoverPopup::applyDocumentStyle()
{
    const bool dark = palette().color(QPalette::Window).lightness() < 128;
    m_browser->document()->setDefaultStyleSheet(hoverStyleSheet(dark));
}

void HoverPopup::showMarkdown(const QString &markdown, const QPoint &globalPos)
{
    if (markdown.trimmed().isEmpty()) {
        hidePopup();
        return;
    }
    applyDocumentStyle();
    m_browser->setHtml(QStringLiteral("<html><body>%1</body></html>")
                           .arg(lspMarkupToHtml(markdown, true)));
    m_browser->document()->setTextWidth(440);
    const int height = qBound(64, int(m_browser->document()->size().height()) + 28, 320);
    resize(460, height);
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

bool HoverPopup::isInsidePopup(QObject *watched) const
{
    for (QObject *obj = watched; obj; obj = obj->parent()) {
        if (obj == this) {
            return true;
        }
    }
    return false;
}

bool HoverPopup::eventFilter(QObject *watched, QEvent *event)
{
    if (!isVisible() || !event) {
        return QFrame::eventFilter(watched, event);
    }
    const bool inside = isInsidePopup(watched);
    switch (event->type()) {
    case QEvent::MouseButtonPress:
        if (!inside) {
            hidePopup();
        }
        break;
    case QEvent::Wheel:
        if (!inside) {
            hidePopup();
        }
        break;
    case QEvent::KeyPress:
        hidePopup();
        break;
    default:
        break;
    }
    return QFrame::eventFilter(watched, event);
}
