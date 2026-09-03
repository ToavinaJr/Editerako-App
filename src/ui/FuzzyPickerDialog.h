#ifndef EDITERAKO_FUZZYPICKERDIALOG_H
#define EDITERAKO_FUZZYPICKERDIALOG_H

#include <QDialog>
#include <QList>
#include <QString>

class QLabel;
class QLineEdit;
class QListWidget;

struct FuzzyPickerItem {
    QString id;
    QString display;
    QString filterText;
    QString hint;
    bool enabled = true;
};

class FuzzyPickerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FuzzyPickerDialog(QWidget *parent = nullptr);

    void setPlaceholderText(const QString &text);
    void setStatusText(const QString &text);
    void setItems(const QList<FuzzyPickerItem> &items);

    [[nodiscard]] QString selectedId() const;
    [[nodiscard]] QString query() const;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    [[nodiscard]] virtual QString rankQuery() const;

private:
    void refreshList();
    void acceptCurrent();
    void moveSelection(int delta);

    QLineEdit *m_edit = nullptr;
    QListWidget *m_list = nullptr;
    QLabel *m_status = nullptr;
    QList<FuzzyPickerItem> m_items;
};

#endif
