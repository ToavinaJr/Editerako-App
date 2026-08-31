#include "editor/EditorDocument.h"

#include "codeeditor.h"

#include <QDir>
#include <QFileInfo>
#include <QTextDocument>

EditorDocument::EditorDocument(CodeEditor *editor)
    : QObject(editor)
    , m_editor(editor)
{
    Q_ASSERT(editor);
    connect(editor->document(), &QTextDocument::modificationChanged,
            this, &EditorDocument::modificationChanged);
}

EditorDocument *EditorDocument::fromEditor(CodeEditor *editor)
{
    if (!editor) {
        return nullptr;
    }
    return editor->findChild<EditorDocument *>(QString(), Qt::FindDirectChildrenOnly);
}

QString EditorDocument::normalizePath(const QString &path)
{
    if (path.isEmpty()) {
        return {};
    }
    const QFileInfo info(path);
    const QString canonical = info.canonicalFilePath();
    if (!canonical.isEmpty()) {
        return canonical;
    }
    return QDir::cleanPath(info.absoluteFilePath());
}

void EditorDocument::setFilePath(const QString &path)
{
    const QString normalized = normalizePath(path);
    if (m_filePath == normalized) {
        return;
    }
    m_filePath = normalized;
    emit filePathChanged(m_filePath);
}

QString EditorDocument::displayName() const
{
    if (m_filePath.isEmpty()) {
        return tr("untitled");
    }
    return QFileInfo(m_filePath).fileName();
}

bool EditorDocument::isUntitled() const
{
    return m_filePath.isEmpty();
}

bool EditorDocument::isModified() const
{
    return m_editor && m_editor->document()->isModified();
}
