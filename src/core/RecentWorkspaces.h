#ifndef EDITERAKO_RECENTWORKSPACES_H
#define EDITERAKO_RECENTWORKSPACES_H

#include <QString>
#include <QStringList>
#include <memory>

class QSettings;

class RecentWorkspaces
{
public:
    static constexpr int MaxEntries = 10;

    RecentWorkspaces();
    explicit RecentWorkspaces(QSettings &settings);
    ~RecentWorkspaces();

    RecentWorkspaces(const RecentWorkspaces &) = delete;
    RecentWorkspaces &operator=(const RecentWorkspaces &) = delete;
    RecentWorkspaces(RecentWorkspaces &&) = delete;
    RecentWorkspaces &operator=(RecentWorkspaces &&) = delete;

    [[nodiscard]] QStringList entries() const;
    QStringList prune();
    void remember(const QString &path);
    void remove(const QString &path);
    void clear();

    [[nodiscard]] static QString normalize(const QString &path);
    [[nodiscard]] static bool samePath(const QString &a, const QString &b);

private:
    void persist(const QStringList &paths);
    [[nodiscard]] static QStringList without(const QStringList &paths, const QString &path);

    std::unique_ptr<QSettings> m_owned;
    QSettings *m_settings = nullptr;
};

#endif
