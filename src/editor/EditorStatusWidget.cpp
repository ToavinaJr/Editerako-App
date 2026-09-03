#include "editor/EditorStatusWidget.h"

#include "core/TextFileFormat.h"
#include "editor/CodeEditor.h"
#include "editor/EditorDocument.h"
#include "syntax/LanguageRegistry.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QTextCursor>

EditorStatusWidget::EditorStatusWidget(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("editorStatusWidget"));

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 0, 8, 0);
    layout->setSpacing(12);

    auto makeLabel = [layout](const QString &name) {
        auto *label = new QLabel;
        label->setObjectName(name);
        layout->addWidget(label);
        return label;
    };

    m_position = makeLabel(QStringLiteral("editorStatusPosition"));
    m_encoding = makeLabel(QStringLiteral("editorStatusEncoding"));
    m_eol = makeLabel(QStringLiteral("editorStatusEol"));
    m_language = makeLabel(QStringLiteral("editorStatusLanguage"));
}

void EditorStatusWidget::setEditor(CodeEditor *editor)
{
    if (m_editor) {
        disconnect(m_editor, nullptr, this, nullptr);
        if (auto *doc = EditorDocument::fromEditor(m_editor)) {
            disconnect(doc, nullptr, this, nullptr);
        }
    }

    m_editor = editor;
    if (m_editor) {
        connect(m_editor, &CodeEditor::cursorPositionChanged, this, &EditorStatusWidget::refresh);
        if (auto *doc = EditorDocument::fromEditor(m_editor)) {
            connect(doc, &EditorDocument::filePathChanged, this, [this](const QString &) {
                refresh();
            });
            connect(doc, &EditorDocument::formatChanged, this, &EditorStatusWidget::refresh);
        }
    }
    refresh();
}

void EditorStatusWidget::refresh()
{
    if (!m_editor) {
        m_position->clear();
        m_encoding->clear();
        m_eol->clear();
        m_language->clear();
        return;
    }

    const QTextCursor cursor = m_editor->textCursor();
    m_position->setText(tr("Ln %1, Col %2")
                            .arg(cursor.blockNumber() + 1)
                            .arg(cursor.positionInBlock() + 1));

    auto *doc = EditorDocument::fromEditor(m_editor);
    const TextFileMeta meta = doc ? doc->format() : defaultTextFileMeta();
    m_encoding->setText(encodingDisplayName(meta.encoding, meta.bom));
    m_eol->setText(lineEndingDisplayName(meta.lineEnding));
    m_language->setText(LanguageRegistry::displayName(doc ? doc->language() : LanguageId::PlainText));
}
