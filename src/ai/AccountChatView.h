#ifndef EDITERAKO_ACCOUNTCHATVIEW_H
#define EDITERAKO_ACCOUNTCHATVIEW_H

#include <QUrl>
#include <QWidget>

class QLabel;
class QPushButton;

class AccountChatView : public QWidget
{
    Q_OBJECT

public:
    explicit AccountChatView(QWidget *parent = nullptr);
    ~AccountChatView() override;

    void navigate(const QUrl &url);
    [[nodiscard]] QUrl currentUrl() const { return m_url; }
    [[nodiscard]] static bool embeddingAvailable();

public slots:
    void openInSystemBrowser();

signals:
    void embeddingFailed(const QString &message);

    friend class AccountChatEnvHandler;
    friend class AccountChatControllerHandler;

protected:
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    void ensureHost();
    void showFallback(const QString &message);
    void onEmbedded();
    void syncBounds();

    struct Impl;
    Impl *m_impl = nullptr;
    QUrl m_url;
    QWidget *m_host = nullptr;
    QWidget *m_fallback = nullptr;
    QLabel *m_fallbackLabel = nullptr;
};

#endif
