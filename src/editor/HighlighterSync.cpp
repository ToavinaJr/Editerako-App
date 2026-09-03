#include "editor/HighlighterSync.h"

#include "core/AppSettings.h"
#include "core/Logging.h"
#include "editor/CodeEditor.h"
#include "editor/EditorDocument.h"
#include "syntax/SyntaxHighlighter.h"

#include <QFileInfo>
#include <QTextDocument>

bool HighlighterSync::shouldHighlight(LanguageId language, qint64 sizeBytes, qint64 disableThreshold)
{
    if (sizeBytes > disableThreshold) {
        return false;
    }
    return LanguageRegistry::tsLanguage(language) != nullptr;
}

void HighlighterSync::apply(CodeEditor *editor)
{
    if (!editor) {
        return;
    }

    auto *edDoc = EditorDocument::fromEditor(editor);
    const QString path = edDoc ? edDoc->filePath() : QString();
    const LanguageId lang = LanguageRegistry::idForPath(path);

    qint64 size = static_cast<qint64>(editor->document()->characterCount());
    if (!path.isEmpty()) {
        const QFileInfo info(path);
        if (info.exists()) {
            size = info.size();
        }
    }

    const auto existing = editor->document()->findChildren<SyntaxHighlighter *>(
        QString(), Qt::FindDirectChildrenOnly);
    const qint64 threshold = AppSettings().largeFileDisableSyntaxBytes();

    if (!shouldHighlight(lang, size, threshold)) {
        for (SyntaxHighlighter *highlighter : existing) {
            delete highlighter;
        }
        if (size > threshold) {
            qCInfo(lcEditor) << "Skipping syntax highlighter for large file" << path << size;
        }
        return;
    }

    if (existing.size() == 1 && existing.front()->language() == lang) {
        return;
    }

    for (SyntaxHighlighter *highlighter : existing) {
        delete highlighter;
    }

    qCInfo(lcEditor) << "Highlighter" << LanguageRegistry::displayName(lang) << "for" << path;
    new SyntaxHighlighter(editor->document(), lang);
}
