#ifndef EDITERAKO_COMPLETIONMODEL_H
#define EDITERAKO_COMPLETIONMODEL_H

#include <QAbstractListModel>
#include <QString>
#include <QVector>

struct CompletionItem {
    QString label;
    QString detail;
    QString documentation;
    QString insertText;
    QString sortText;
    int kind = 0;
    bool hasTextEdit = false;
    int startLine = 0;
    int startCharacter = 0;
    int endLine = 0;
    int endCharacter = 0;
};

class CompletionModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit CompletionModel(QObject *parent = nullptr);

    void setItems(const QVector<CompletionItem> &items);
    void setFilter(const QString &prefix);
    [[nodiscard]] QString filter() const { return m_filter; }
    [[nodiscard]] CompletionItem itemAt(int row) const;
    [[nodiscard]] int visibleCount() const { return m_visible.size(); }

    [[nodiscard]] int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

private:
    void rebuild();

    QVector<CompletionItem> m_all;
    QVector<CompletionItem> m_visible;
    QString m_filter;
};

[[nodiscard]] QString completionPrefixAtCursor(const class QTextCursor &cursor);
void applyCompletionItem(class QPlainTextEdit *editor, const CompletionItem &item);

#endif
