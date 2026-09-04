#include "editor/CompletionModel.h"

#include "editor/EditorDiagnostic.h"

#include <algorithm>
#include <QPlainTextEdit>
#include <QTextCursor>
#include <QTextDocument>

CompletionModel::CompletionModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

void CompletionModel::setItems(const QVector<CompletionItem> &items)
{
    beginResetModel();
    m_all = items;
    rebuild();
    endResetModel();
}

void CompletionModel::setFilter(const QString &prefix)
{
    if (m_filter == prefix) {
        return;
    }
    beginResetModel();
    m_filter = prefix;
    rebuild();
    endResetModel();
}

CompletionItem CompletionModel::itemAt(int row) const
{
    if (row < 0 || row >= m_visible.size()) {
        return {};
    }
    return m_visible.at(row);
}

int CompletionModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_visible.size();
}

QVariant CompletionModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_visible.size()) {
        return {};
    }
    const CompletionItem &item = m_visible.at(index.row());
    if (role == Qt::DisplayRole) {
        if (item.detail.isEmpty()) {
            return item.label;
        }
        return item.label + QStringLiteral("  ") + item.detail;
    }
    if (role == Qt::ToolTipRole) {
        QString tip = item.detail;
        if (!item.documentation.isEmpty()) {
            if (!tip.isEmpty()) {
                tip += QLatin1Char('\n');
            }
            tip += item.documentation;
        }
        return tip;
    }
    return {};
}

namespace {

bool completionItemMatches(const CompletionItem &item, const QString &needle)
{
    return item.label.contains(needle, Qt::CaseInsensitive)
        || item.insertText.contains(needle, Qt::CaseInsensitive)
        || item.filterText.contains(needle, Qt::CaseInsensitive);
}

} // namespace

void CompletionModel::rebuild()
{
    m_visible.clear();
    const QString needle = m_filter.trimmed();
    for (const CompletionItem &item : m_all) {
        if (needle.isEmpty() || completionItemMatches(item, needle)) {
            m_visible.append(item);
        }
    }
    std::sort(m_visible.begin(), m_visible.end(), [](const CompletionItem &a, const CompletionItem &b) {
        const QString as = a.sortText.isEmpty() ? a.label : a.sortText;
        const QString bs = b.sortText.isEmpty() ? b.label : b.sortText;
        const int cmp = QString::compare(as, bs, Qt::CaseInsensitive);
        if (cmp != 0) {
            return cmp < 0;
        }
        return QString::compare(a.label, b.label, Qt::CaseInsensitive) < 0;
    });
}

QString completionPrefixAtCursor(const QTextCursor &cursor)
{
    QTextCursor word = cursor;
    word.movePosition(QTextCursor::StartOfWord, QTextCursor::KeepAnchor);
    return word.selectedText();
}

void applyCompletionItem(QPlainTextEdit *editor, const CompletionItem &item)
{
    if (!editor) {
        return;
    }
    const QString text = item.insertText.isEmpty() ? item.label : item.insertText;
    QTextCursor cursor = editor->textCursor();
    if (item.hasTextEdit) {
        QTextDocument *doc = editor->document();
        const int start = documentPositionAt(doc, item.startLine, item.startCharacter);
        const int end = documentPositionAt(doc, item.endLine, item.endCharacter);
        cursor.setPosition(start);
        cursor.setPosition(qMax(start, end), QTextCursor::KeepAnchor);
        cursor.insertText(text);
        editor->setTextCursor(cursor);
        return;
    }
    cursor.movePosition(QTextCursor::StartOfWord, QTextCursor::KeepAnchor);
    cursor.insertText(text);
    editor->setTextCursor(cursor);
}
