#include "viewers/ViewerManager.h"

#include "core/Logging.h"
#include "editor/EditorManager.h"
#include "viewers/FileKind.h"
#include "viewers/ImageViewer.h"
#include "viewers/PdfViewer.h"

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

    switch (kindForPath(filePath)) {
    case FileKind::Text:
        return m_editors->openTextFile(filePath) ? OpenResult::Opened : OpenResult::Failed;
    case FileKind::Pdf:
        return openPdf(filePath);
    case FileKind::Image:
        return openImage(filePath);
    case FileKind::Unsupported:
        qCInfo(lcViewer) << "Unsupported file type" << filePath;
        return OpenResult::Unsupported;
    }
    return OpenResult::Unsupported;
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
