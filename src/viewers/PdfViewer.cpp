#include "viewers/PdfViewer.h"

#include "core/Logging.h"

#include <QLabel>
#include <QPdfView>
#include <QSizePolicy>
#include <QVBoxLayout>

PdfViewer::PdfViewer(QWidget *parent)
    : QWidget(parent)
    , m_document(new QPdfDocument(this))
    , m_view(new QPdfView(this))
    , m_statusLabel(new QLabel(this))
{
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setStyleSheet(QStringLiteral("color: #cccccc; background-color: #1e1e1e;"));
    m_statusLabel->setText(tr("Loading PDF…"));

    m_view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_view->hide();

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_statusLabel);
    layout->addWidget(m_view, 1);

    connect(m_document, &QPdfDocument::statusChanged,
            this, &PdfViewer::onStatusChanged, Qt::QueuedConnection);
}

bool PdfViewer::load(const QString &filePath)
{
    m_filePath = filePath;
    m_statusLabel->setText(tr("Loading PDF…"));
    m_statusLabel->show();
    m_view->hide();
    if (m_view->document() == m_document) {
        m_view->setDocument(nullptr);
    }

    const QPdfDocument::Error err = m_document->load(filePath);
    if (err != QPdfDocument::Error::None && err != QPdfDocument::Error::DataNotYetAvailable) {
        qCWarning(lcViewer) << "PDF load failed" << filePath << static_cast<int>(err);
        showError(tr("Could not open PDF:\n%1").arg(filePath));
        return false;
    }

    if (m_document->status() == QPdfDocument::Status::Ready) {
        attachViewIfReady();
    }
    return true;
}

void PdfViewer::onStatusChanged(QPdfDocument::Status status)
{
    if (status == QPdfDocument::Status::Error) {
        qCWarning(lcViewer) << "PDF status error" << m_filePath
                            << static_cast<int>(m_document->error());
        showError(tr("Failed to open PDF:\n%1").arg(m_filePath));
        return;
    }
    if (status == QPdfDocument::Status::Ready) {
        attachViewIfReady();
    }
}

void PdfViewer::attachViewIfReady()
{
    if (m_document->status() != QPdfDocument::Status::Ready) {
        return;
    }
    if (m_document->pageCount() <= 0) {
        showError(tr("This PDF has no pages:\n%1").arg(m_filePath));
        return;
    }

    if (m_view->document() != m_document) {
        m_view->setDocument(m_document);
    }
    m_statusLabel->hide();
    m_view->show();
    qCInfo(lcViewer) << "PDF ready" << m_filePath << "pages" << m_document->pageCount();
}

void PdfViewer::showError(const QString &message)
{
    if (m_view->document() == m_document) {
        m_view->setDocument(nullptr);
    }
    m_view->hide();
    m_statusLabel->setText(message);
    m_statusLabel->show();
}
