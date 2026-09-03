#ifndef EDITERAKO_SESSIONCONTROLLER_H
#define EDITERAKO_SESSIONCONTROLLER_H

#include "core/SessionStore.h"

class SessionController
{
public:
    class RestoreGuard
    {
    public:
        explicit RestoreGuard(SessionController &controller);
        ~RestoreGuard();

        RestoreGuard(const RestoreGuard &) = delete;
        RestoreGuard &operator=(const RestoreGuard &) = delete;

    private:
        SessionController &m_controller;
    };

    [[nodiscard]] bool isRestoring() const { return m_restoring; }

    void save(const SessionState &state);
    [[nodiscard]] SessionState load() const;
    void save(const SessionState &state, QSettings &settings);
    [[nodiscard]] SessionState load(QSettings &settings) const;

    [[nodiscard]] static bool workspaceIsRestorable(const SessionState &state);
    [[nodiscard]] static QStringList existingFiles(const QStringList &paths);

private:
    friend class RestoreGuard;

    void beginRestore();
    void endRestore();

    SessionStore m_store;
    bool m_restoring = false;
};

#endif
