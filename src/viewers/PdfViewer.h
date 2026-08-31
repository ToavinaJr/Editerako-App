#ifndef EDITERAKO_PDFVIEWER_H
#define EDITERAKO_PDFVIEWER_H

#include <QPdfDocument>
#include <QString>
#include <QWidget>

class QLabel;
class QPdfView;

class PdfViewer : public QWidget
{
    Q_OBJECT

public:
    explicit PdfViewer(QWidget *parent = nullptr);

    bool load(const QString &filePath);
    [[nodiscard]] QString filePath() const { return m_filePath; }

private slots:
    void onStatusChanged(QPdfDocument::Status status);

private:
    void showError(const QString &message);
    void attachViewIfReady();

    QString m_filePath;
    QPdfDocument *m_document = nullptr;
    QPdfView *m_view = nullptr;
    QLabel *m_statusLabel = nullptr;
};

#endif
