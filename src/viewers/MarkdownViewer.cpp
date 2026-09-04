#include "viewers/MarkdownViewer.h"

#include <QFile>
#include <QFileInfo>
#include <QTextBrowser>
#include <QVBoxLayout>

MarkdownViewer::MarkdownViewer(QWidget *parent)
    : QWidget(parent)
    , m_browser(new QTextBrowser(this))
{
    m_browser->setObjectName(QStringLiteral("markdownPreview"));
    m_browser->setOpenExternalLinks(true);
    m_browser->setReadOnly(true);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->addWidget(m_browser);
}

bool MarkdownViewer::load(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    QByteArray bytes = file.readAll();
    if (bytes.startsWith("\xEF\xBB\xBF")) {
        bytes.remove(0, 3);
    }

    m_filePath = filePath;
    m_browser->setMarkdown(QString::fromUtf8(bytes));
    setWindowTitle(tr("%1 (Preview)").arg(QFileInfo(filePath).fileName()));
    return true;
}
