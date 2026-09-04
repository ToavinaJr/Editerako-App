#include "viewers/ViewerManager.h"

#include "core/Logging.h"
#include "editor/EditorDocument.h"
#include "editor/EditorManager.h"
#include "plugins/IFileViewerProvider.h"
#include "viewers/CsvViewer.h"
#include "viewers/FileKind.h"
#include "viewers/ImageViewer.h"
#include "viewers/MarkdownViewer.h"
#include "viewers/PdfViewer.h"
#include "viewers/SvgViewer.h"

#include <QFileInfo>
#include <QMessageBox>
#include <QTabWidget>
#include <QWidget>

ViewerManager::ViewerManager(EditorManager *editors, QObject *parent)
    : QObject(parent)
    , m_editors(editors)
{
}

ViewerManager::FileKind ViewerManager::kindForPath(const QString &path)
{
    return fileKindForPath(path);
}

ViewerManager::OpenResult ViewerManager::open(const QString &filePath)
{
    if (!m_editors || filePath.isEmpty()) {
        return OpenResult::Failed;
    }

    if (m_editors->activateExisting(filePath)) {
        return OpenResult::Opened;
    }

    return openNew(filePath);
}

ViewerManager::OpenResult ViewerManager::openNew(const QString &filePath)
{
    if (!m_editors || filePath.isEmpty()) {
        return OpenResult::Failed;
    }

    for (IFileViewerProvider *provider : m_providers) {
        if (provider && provider->canOpen(filePath)) {
            const OpenResult result = openWithProvider(provider, filePath);
            if (result != OpenResult::Unsupported) {
                return result;
            }
        }
    }

    switch (kindForPath(filePath)) {
    case FileKind::Text:
        return m_editors->openTextFile(filePath) ? OpenResult::Opened : OpenResult::Failed;
    case FileKind::Pdf:
        return openPdf(filePath);
    case FileKind::Image:
        return openImage(filePath);
    case FileKind::Svg:
        return openSvg(filePath);
    case FileKind::Csv:
        return openCsv(filePath);
    case FileKind::Unsupported:
        qCInfo(lcViewer) << "Unsupported file type" << filePath;
        return OpenResult::Unsupported;
    }
    return OpenResult::Unsupported;
}

ViewerManager::OpenResult ViewerManager::openMarkdownPreview(const QString &filePath)
{
    if (!m_editors || filePath.isEmpty() || !isMarkdownPath(filePath)) {
        return OpenResult::Failed;
    }

    const QString normalized = EditorDocument::normalizePath(filePath);
    for (QWidget *widget : m_editors->tabWidgets()) {
        if (auto *preview = qobject_cast<MarkdownViewer *>(widget)) {
            if (EditorDocument::normalizePath(preview->filePath()) == normalized) {
                m_editors->activateWidget(widget);
                return OpenResult::Opened;
            }
        }
    }

    auto *viewer = new MarkdownViewer(m_editors->tabWidget());
    if (!viewer->load(filePath)) {
        delete viewer;
        QMessageBox::warning(qobject_cast<QWidget *>(parent()),
                             tr("Error"),
                             tr("Could not open Markdown preview:\n%1").arg(filePath));
        return OpenResult::Failed;
    }

    m_editors->addViewerTab(viewer, filePath);
    qCInfo(lcViewer) << "Opened Markdown preview" << filePath;
    return OpenResult::Opened;
}

ViewerManager::OpenResult ViewerManager::openPdf(const QString &filePath)
{
    if (!QFileInfo::exists(filePath)) {
        QMessageBox::warning(qobject_cast<QWidget *>(parent()),
                             tr("Error"),
                             tr("Could not open PDF:\n%1").arg(filePath));
        return OpenResult::Failed;
    }

    auto *viewer = new PdfViewer(m_editors->tabWidget());
    m_editors->addViewerTab(viewer, filePath);
    viewer->load(filePath);
    qCInfo(lcViewer) << "Opened PDF" << filePath;
    return OpenResult::Opened;
}

ViewerManager::OpenResult ViewerManager::openImage(const QString &filePath)
{
    auto *viewer = new ImageViewer(m_editors->tabWidget());
    if (!viewer->load(filePath)) {
        delete viewer;
        QMessageBox::warning(qobject_cast<QWidget *>(parent()),
                             tr("Error"),
                             tr("Could not open image:\n%1").arg(filePath));
        return OpenResult::Failed;
    }

    m_editors->addViewerTab(viewer, filePath);
    qCInfo(lcViewer) << "Opened image" << filePath;
    return OpenResult::Opened;
}

ViewerManager::OpenResult ViewerManager::openSvg(const QString &filePath)
{
    auto *viewer = new SvgViewer(m_editors->tabWidget());
    if (!viewer->load(filePath)) {
        delete viewer;
        QMessageBox::warning(qobject_cast<QWidget *>(parent()),
                             tr("Error"),
                             tr("Could not open SVG:\n%1").arg(filePath));
        return OpenResult::Failed;
    }

    m_editors->addViewerTab(viewer, filePath);
    qCInfo(lcViewer) << "Opened SVG" << filePath;
    return OpenResult::Opened;
}

ViewerManager::OpenResult ViewerManager::openCsv(const QString &filePath)
{
    auto *viewer = new CsvViewer(m_editors->tabWidget());
    if (!viewer->load(filePath)) {
        delete viewer;
        QMessageBox::warning(qobject_cast<QWidget *>(parent()),
                             tr("Error"),
                             tr("Could not open CSV:\n%1").arg(filePath));
        return OpenResult::Failed;
    }

    m_editors->addViewerTab(viewer, filePath);
    qCInfo(lcViewer) << "Opened CSV" << filePath;
    return OpenResult::Opened;
}

void ViewerManager::addProvider(IFileViewerProvider *provider)
{
    if (provider && !m_providers.contains(provider)) {
        m_providers.prepend(provider);
    }
}

void ViewerManager::removeProvider(IFileViewerProvider *provider)
{
    m_providers.removeAll(provider);
}

ViewerManager::OpenResult ViewerManager::openWithProvider(IFileViewerProvider *provider,
                                                          const QString &filePath)
{
    QString error;
    QWidget *widget = provider->create(filePath, m_editors->tabWidget(), &error);
    if (!widget) {
        if (!error.isEmpty()) {
            QMessageBox::warning(qobject_cast<QWidget *>(parent()), tr("Error"), error);
        }
        return OpenResult::Failed;
    }
    m_editors->addViewerTab(widget, filePath);
    qCInfo(lcViewer) << "Opened with plugin viewer" << provider->id() << filePath;
    return OpenResult::Opened;
}
