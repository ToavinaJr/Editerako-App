#include "editor/EditorStatusWidget.h"

#include "core/AppSettings.h"
#include "core/TextFileFormat.h"
#include "editor/CodeEditor.h"
#include "editor/EditorDocument.h"
#include "editor/StatusBarText.h"
#include "syntax/LanguageRegistry.h"

#include <QBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextCursor>

namespace {

QLabel *makeSegment(QWidget *parent, QBoxLayout *layout, const QString &objectName,
                    const QString &tooltip)
{
    auto *label = new QLabel(parent);
    label->setObjectName(objectName);
    label->setToolTip(tooltip);
    label->hide();
    layout->addWidget(label);
    return label;
}

} // namespace

EditorStatusWidget::EditorStatusWidget(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("editorStatusWidget"));

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 0, 8, 0);
    layout->setSpacing(12);

    m_position = makeSegment(this, layout, QStringLiteral("editorStatusPosition"),
                             tr("Line and column"));
    m_indentMode = makeSegment(this, layout, QStringLiteral("editorStatusIndentMode"),
                               tr("Insert spaces or tabs"));
    m_tabSize = makeSegment(this, layout, QStringLiteral("editorStatusTabSize"),
                            tr("Tab size"));
    m_encoding = makeSegment(this, layout, QStringLiteral("editorStatusEncoding"),
                             tr("File encoding"));
    m_eol = makeSegment(this, layout, QStringLiteral("editorStatusEol"),
                        tr("Line ending"));
    m_language = makeSegment(this, layout, QStringLiteral("editorStatusLanguage"),
                             tr("Language"));
    m_git = makeSegment(this, layout, QStringLiteral("editorStatusGit"),
                        tr("Git branch"));
    m_lsp = makeSegment(this, layout, QStringLiteral("editorStatusLsp"),
                        tr("Language server"));
    m_debug = makeSegment(this, layout, QStringLiteral("editorStatusDebug"),
                          tr("Debugger"));

    m_problems = new QPushButton(this);
    m_problems->setObjectName(QStringLiteral("editorStatusProblems"));
    m_problems->setFlat(true);
    m_problems->setCursor(Qt::PointingHandCursor);
    m_problems->setFocusPolicy(Qt::NoFocus);
    m_problems->setToolTip(tr("Problems"));
    layout->addWidget(m_problems);
    connect(m_problems, &QPushButton::clicked, this, &EditorStatusWidget::problemsActivated);
    setProblemCounts(0, 0);
}

void EditorStatusWidget::setSegmentText(QLabel *label, const QString &text)
{
    if (!label) {
        return;
    }
    label->setText(text);
    label->setVisible(!text.isEmpty());
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

void EditorStatusWidget::applySettings()
{
    refresh();
}

void EditorStatusWidget::refresh()
{
    if (!m_editor) {
        setSegmentText(m_position, {});
        setSegmentText(m_indentMode, {});
        setSegmentText(m_tabSize, {});
        setSegmentText(m_encoding, {});
        setSegmentText(m_eol, {});
        setSegmentText(m_language, {});
        return;
    }

    const QTextCursor cursor = m_editor->textCursor();
    setSegmentText(m_position,
                   statusBarPositionLabel(cursor.blockNumber() + 1, cursor.positionInBlock() + 1));

    const AppSettings settings;
    setSegmentText(m_indentMode, statusBarIndentModeLabel(settings.editorInsertSpaces()));
    setSegmentText(m_tabSize, statusBarTabSizeLabel(settings.editorTabSize()));

    auto *doc = EditorDocument::fromEditor(m_editor);
    const TextFileMeta meta = doc ? doc->format() : defaultTextFileMeta();
    setSegmentText(m_encoding, encodingDisplayName(meta.encoding, meta.bom));
    setSegmentText(m_eol, lineEndingDisplayName(meta.lineEnding));

    QString language = LanguageRegistry::displayName(doc ? doc->language() : LanguageId::PlainText);
    if (doc && doc->language() == LanguageId::PlainText) {
        const QString extra = LanguageRegistry::extraDisplayNameForPath(doc->filePath());
        if (!extra.isEmpty()) {
            language = extra;
        }
    }
    setSegmentText(m_language, language);
}

void EditorStatusWidget::setLspStatus(const QString &text)
{
    setSegmentText(m_lsp, text);
}

void EditorStatusWidget::setGitBranch(const QString &text)
{
    setSegmentText(m_git, text);
}

void EditorStatusWidget::setDebugStatus(const QString &text)
{
    setSegmentText(m_debug, text);
}

void EditorStatusWidget::setProblemCounts(int errors, int warnings)
{
    m_problems->setText(statusBarProblemsLabel(errors, warnings));
}
