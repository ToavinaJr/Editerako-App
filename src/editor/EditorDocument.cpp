#include "editor/EditorDocument.h"

#include "editor/CodeEditor.h"

#include <QDir>
#include <QFileInfo>
#include <QScrollBar>
#include <QTextCursor>
#include <QTextDocument>
#include <QtGlobal>
#include <QUuid>

EditorDocument::EditorDocument(CodeEditor *editor)
    : QObject(editor)
    , m_editor(editor)
    , m_format(defaultTextFileMeta())
{
    Q_ASSERT(editor);
    connect(editor->document(), &QTextDocument::modificationChanged,
            this, &EditorDocument::modificationChanged);
    connect(editor->document(), &QTextDocument::contentsChange,
            this, [this](int, int, int) {
                ++m_version;
                emit versionChanged(m_version);
            });
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

void EditorDocument::setFormat(const TextFileMeta &meta)
{
    if (m_format == meta) {
        return;
    }
    m_format = meta;
    emit formatChanged();
}

LanguageId EditorDocument::language() const
{
    return LanguageRegistry::idForPath(m_filePath);
}

void EditorDocument::setReadOnly(bool readOnly)
{
    m_readOnly = readOnly;
}

void EditorDocument::resetVersion()
{
    m_version = 0;
    emit versionChanged(m_version);
}

EditorDocument::CaretState EditorDocument::caretState() const
{
    CaretState state;
    if (!m_editor) {
        return state;
    }
    const QTextCursor cursor = m_editor->textCursor();
    state.position = cursor.position();
    state.anchor = cursor.anchor();
    if (auto *bar = m_editor->verticalScrollBar()) {
        state.scrollY = bar->value();
    }
    return state;
}

void EditorDocument::setBackupId(const QString &id)
{
    m_backupId = id;
}

QString EditorDocument::ensureBackupId()
{
    if (m_backupId.isEmpty()) {
        m_backupId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
    return m_backupId;
}

void EditorDocument::restoreCaretState(const CaretState &state)
{
    if (!m_editor) {
        return;
    }
    QTextCursor cursor(m_editor->document());
    const int length = m_editor->document()->characterCount() - 1;
    const int pos = qBound(0, state.position, length);
    const int anchor = qBound(0, state.anchor, length);
    cursor.setPosition(anchor);
    cursor.setPosition(pos, QTextCursor::KeepAnchor);
    m_editor->setTextCursor(cursor);
    if (auto *bar = m_editor->verticalScrollBar()) {
        bar->setValue(state.scrollY);
    }
}
