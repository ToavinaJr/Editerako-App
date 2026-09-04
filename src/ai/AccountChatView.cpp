#include "ai/AccountChatView.h"

#include <QDesktopServices>
#include <QDir>
#include <QLabel>
#include <QPushButton>
#include <QResizeEvent>
#include <QShowEvent>
#include <QStandardPaths>
#include <QVBoxLayout>

#ifdef Q_OS_WIN
#include <windows.h>
#ifdef interface
#undef interface
#endif
#endif

#if defined(Q_OS_WIN) && defined(EDITERAKO_HAS_WEBVIEW2)
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#endif
#include <WebView2.h>
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
#include <cstring>
#include <string>
#endif

struct AccountChatView::Impl {
#if defined(Q_OS_WIN) && defined(EDITERAKO_HAS_WEBVIEW2)
    ICoreWebView2Controller *controller = nullptr;
    ICoreWebView2 *webview = nullptr;
    HWND hwnd = nullptr;
    bool creating = false;
#endif
};

#if defined(Q_OS_WIN) && defined(EDITERAKO_HAS_WEBVIEW2)

namespace {

using CreateEnvFn = HRESULT(STDAPICALLTYPE *)(PCWSTR, PCWSTR, ICoreWebView2EnvironmentOptions *,
                                              ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler *);

CreateEnvFn loadCreateEnv()
{
    HMODULE module = LoadLibraryW(L"WebView2Loader.dll");
    if (!module) {
        return nullptr;
    }
    CreateEnvFn fn = nullptr;
    const FARPROC raw = GetProcAddress(module, "CreateCoreWebView2EnvironmentWithOptions");
    std::memcpy(&fn, &raw, sizeof(raw));
    return fn;
}

std::wstring profileFolder()
{
    const QString root =
        QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    const QString path = QDir(root).filePath(QStringLiteral("webview-profile"));
    QDir().mkpath(path);
    return path.toStdWString();
}

} // namespace

class AccountChatControllerHandler final : public ICoreWebView2CreateCoreWebView2ControllerCompletedHandler
{
public:
    AccountChatControllerHandler(AccountChatView *view, AccountChatView::Impl *impl)
        : m_view(view)
        , m_impl(impl)
    {
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppv) override
    {
        if (riid == IID_IUnknown
            || riid == IID_ICoreWebView2CreateCoreWebView2ControllerCompletedHandler) {
            *ppv = static_cast<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler *>(this);
            AddRef();
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&m_ref); }
    ULONG STDMETHODCALLTYPE Release() override
    {
        const ULONG value = InterlockedDecrement(&m_ref);
        if (value == 0) {
            delete this;
        }
        return value;
    }
    HRESULT STDMETHODCALLTYPE Invoke(HRESULT result, ICoreWebView2Controller *controller) override;

private:
    LONG m_ref = 1;
    AccountChatView *m_view = nullptr;
    AccountChatView::Impl *m_impl = nullptr;
};

class AccountChatEnvHandler final : public ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler
{
public:
    AccountChatEnvHandler(AccountChatView *view, AccountChatView::Impl *impl)
        : m_view(view)
        , m_impl(impl)
    {
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppv) override
    {
        if (riid == IID_IUnknown
            || riid == IID_ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler) {
            *ppv = static_cast<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler *>(this);
            AddRef();
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&m_ref); }
    ULONG STDMETHODCALLTYPE Release() override
    {
        const ULONG value = InterlockedDecrement(&m_ref);
        if (value == 0) {
            delete this;
        }
        return value;
    }
    HRESULT STDMETHODCALLTYPE Invoke(HRESULT result, ICoreWebView2Environment *env) override
    {
        if (!m_impl) {
            return S_OK;
        }
        m_impl->creating = false;
        if (FAILED(result) || !env || !m_impl->hwnd) {
            if (m_view) {
                m_view->showFallback(AccountChatView::tr(
                    "Could not start the embedded browser. Sign in in your system browser."));
            }
            return result;
        }
        return env->CreateCoreWebView2Controller(m_impl->hwnd,
                                                 new AccountChatControllerHandler(m_view, m_impl));
    }

private:
    LONG m_ref = 1;
    AccountChatView *m_view = nullptr;
    AccountChatView::Impl *m_impl = nullptr;
};

HRESULT AccountChatControllerHandler::Invoke(HRESULT result, ICoreWebView2Controller *controller)
{
    if (!m_impl) {
        return S_OK;
    }
    if (FAILED(result) || !controller) {
        if (m_view) {
            m_view->showFallback(AccountChatView::tr(
                "Could not attach the embedded browser. Sign in in your system browser."));
        }
        return result;
    }
    if (m_impl->controller) {
        m_impl->controller->Release();
    }
    m_impl->controller = controller;
    controller->AddRef();
    controller->put_IsVisible(TRUE);
    if (m_impl->webview) {
        m_impl->webview->Release();
        m_impl->webview = nullptr;
    }
    controller->get_CoreWebView2(&m_impl->webview);
    if (m_view) {
        m_view->onEmbedded();
        m_view->syncBounds();
        const QUrl url = m_view->currentUrl();
        if (url.isValid() && m_impl->webview) {
            const std::wstring href = url.toString().toStdWString();
            m_impl->webview->Navigate(href.c_str());
        }
    }
    return S_OK;
}

#endif

AccountChatView::AccountChatView(QWidget *parent)
    : QWidget(parent)
    , m_impl(new Impl)
{
    setObjectName(QStringLiteral("accountChatView"));
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_host = new QWidget(this);
    m_host->setObjectName(QStringLiteral("accountChatHost"));
    m_host->setAttribute(Qt::WA_NativeWindow);
    m_host->setAttribute(Qt::WA_DontCreateNativeAncestors);
    layout->addWidget(m_host, 1);

    m_fallback = new QWidget(this);
    auto *fall = new QVBoxLayout(m_fallback);
    m_fallbackLabel = new QLabel(m_fallback);
    m_fallbackLabel->setWordWrap(true);
    m_fallbackLabel->setObjectName(QStringLiteral("accountChatFallback"));
    auto *openBtn = new QPushButton(tr("Sign in in the browser"), m_fallback);
    connect(openBtn, &QPushButton::clicked, this, &AccountChatView::openInSystemBrowser);
    fall->addWidget(m_fallbackLabel);
    fall->addWidget(openBtn);
    fall->addStretch(1);
    layout->addWidget(m_fallback);
    m_fallback->hide();
}

AccountChatView::~AccountChatView()
{
#if defined(Q_OS_WIN) && defined(EDITERAKO_HAS_WEBVIEW2)
    if (m_impl) {
        if (m_impl->webview) {
            m_impl->webview->Release();
        }
        if (m_impl->controller) {
            m_impl->controller->Close();
            m_impl->controller->Release();
        }
    }
#endif
    delete m_impl;
}

bool AccountChatView::embeddingAvailable()
{
#if defined(Q_OS_WIN) && defined(EDITERAKO_HAS_WEBVIEW2)
    return loadCreateEnv() != nullptr;
#else
    return false;
#endif
}

void AccountChatView::showFallback(const QString &message)
{
    m_fallbackLabel->setText(message);
    m_fallback->show();
    if (m_host) {
        m_host->hide();
    }
    emit embeddingFailed(message);
}

void AccountChatView::onEmbedded()
{
    m_fallback->hide();
    if (m_host) {
        m_host->show();
    }
}

void AccountChatView::openInSystemBrowser()
{
    if (m_url.isValid()) {
        QDesktopServices::openUrl(m_url);
    }
}

void AccountChatView::navigate(const QUrl &url)
{
    m_url = url;
    if (!url.isValid()) {
        return;
    }
#if defined(Q_OS_WIN) && defined(EDITERAKO_HAS_WEBVIEW2)
    ensureHost();
    if (m_impl && m_impl->webview) {
        const std::wstring href = url.toString().toStdWString();
        m_impl->webview->Navigate(href.c_str());
        m_fallback->hide();
        return;
    }
#endif
    if (!embeddingAvailable()) {
        showFallback(tr("Sign in with your account in the system browser. Your existing chats stay "
                        "on that account — no Gemini API key is required."));
    }
}

void AccountChatView::ensureHost()
{
#if defined(Q_OS_WIN) && defined(EDITERAKO_HAS_WEBVIEW2)
    if (!m_impl || m_impl->controller || m_impl->creating) {
        return;
    }
    CreateEnvFn create = loadCreateEnv();
    if (!create) {
        showFallback(tr("WebView2 loader not found. Sign in in your system browser."));
        return;
    }
    winId();
    if (m_host) {
        m_host->winId();
        m_impl->hwnd = reinterpret_cast<HWND>(m_host->winId());
    }
    if (!m_impl->hwnd) {
        return;
    }
    m_impl->creating = true;
    const std::wstring folder = profileFolder();
    const HRESULT hr =
        create(nullptr, folder.c_str(), nullptr, new AccountChatEnvHandler(this, m_impl));
    if (FAILED(hr)) {
        m_impl->creating = false;
        showFallback(tr("Could not create the embedded browser. Sign in in your system browser."));
    }
#else
    Q_UNUSED(this)
#endif
}

void AccountChatView::syncBounds()
{
#if defined(Q_OS_WIN) && defined(EDITERAKO_HAS_WEBVIEW2)
    if (!m_impl || !m_impl->controller || !m_impl->hwnd) {
        return;
    }
    RECT bounds{};
    GetClientRect(m_impl->hwnd, &bounds);
    m_impl->controller->put_Bounds(bounds);
#endif
}

void AccountChatView::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    syncBounds();
}

void AccountChatView::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    ensureHost();
    if (m_url.isValid() && m_impl) {
#if defined(Q_OS_WIN) && defined(EDITERAKO_HAS_WEBVIEW2)
        if (m_impl->webview) {
            const std::wstring href = m_url.toString().toStdWString();
            m_impl->webview->Navigate(href.c_str());
        }
#endif
    }
}
