#ifndef EDITERAKO_CSVVIEWER_H
#define EDITERAKO_CSVVIEWER_H

#include <QString>
#include <QWidget>

class QStandardItemModel;
class QTableView;

class CsvViewer : public QWidget
{
public:
    explicit CsvViewer(QWidget *parent = nullptr);

    bool load(const QString &filePath);
    [[nodiscard]] QString filePath() const { return m_filePath; }

private:
    QString m_filePath;
    QTableView *m_table = nullptr;
    QStandardItemModel *m_model = nullptr;
};

#endif
