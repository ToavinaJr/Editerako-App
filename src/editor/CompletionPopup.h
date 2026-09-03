#ifndef EDITERAKO_COMPLETIONPOPUP_H
#define EDITERAKO_COMPLETIONPOPUP_H

#include "editor/CompletionModel.h"

#include <QFrame>

class CodeEditor;
class QListView;

class CompletionPopup : public QFrame
{
    Q_OBJECT

public:
    explicit CompletionPopup(QWidget *parent = nullptr);

    void showItems(CodeEditor *editor, const QVector<CompletionItem> &items, const QString &prefix);
    void updateFilter(const QString &prefix);
    void hidePopup();
    [[nodiscard]] bool isVisibleFor(const CodeEditor *editor) const;

signals:
    void itemActivated(const CompletionItem &item);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void activateCurrent();
    void placeNearCursor();

    CompletionModel *m_model = nullptr;
    QListView *m_view = nullptr;
    CodeEditor *m_editor = nullptr;
};

#endif
