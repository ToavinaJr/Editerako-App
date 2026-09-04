#include "core/RecentWorkspaces.h"

#include "core/Logging.h"

#include <QDir>
#include <QFileInfo>
#include <QSettings>

namespace {

constexpr auto kKey = "recent/workspaces";

Qt::CaseSensitivity pathCase()
{
#ifdef Q_OS_WIN
    return Qt::CaseInsensitive;
#else
    return Qt::CaseSensitive;
#endif
}

} // namespace

RecentWorkspaces::RecentWorkspaces()
    : m_owned(std::make_unique<QSettings>()), m_settings(m_owned.get())
{}

RecentWorkspaces::RecentWorkspaces(QSettings &settings) : m_settings(&settings)
{}

RecentWorkspaces::~RecentWorkspaces() = default;

QString RecentWorkspaces::normalize(const QString &path)
{
    const QString trimmed = path.trimmed();
    if (trimmed.isEmpty()) {
        return {};
    }
    const QFileInfo info(trimmed);
    if (info.exists() && info.isDir()) {
        const QString canonical = info.canonicalFilePath();
        if (!canonical.isEmpty()) {
            return canonical;
        }
    }
    return QDir::cleanPath(info.absoluteFilePath());
}

bool RecentWorkspaces::samePath(const QString &a, const QString &b)
{
    const QString left = QDir::fromNativeSeparators(normalize(a));
    const QString right = QDir::fromNativeSeparators(normalize(b));
    return QString::compare(left, right, pathCase()) == 0;
}

QStringList RecentWorkspaces::without(const QStringList &paths, const QString &path)
{
    QStringList kept;
    kept.reserve(paths.size());
    for (const QString &item : paths) {
        if (!samePath(item, path)) {
            kept.append(item);
        }
    }
    return kept;
}

QStringList RecentWorkspaces::entries() const
{
    if (!m_settings) {
        return {};
    }
    return m_settings->value(QLatin1String(kKey)).toStringList();
}

void RecentWorkspaces::persist(const QStringList &paths)
{
    if (!m_settings) {
        return;
    }
    m_settings->setValue(QLatin1String(kKey), paths);
    m_settings->sync();
}

QStringList RecentWorkspaces::prune()
{
    QStringList kept;
    for (const QString &item : entries()) {
        const QString normalized = normalize(item);
        if (normalized.isEmpty() || !QDir(normalized).exists()) {
            continue;
        }
        bool duplicate = false;
        for (const QString &existing : kept) {
            if (samePath(existing, normalized)) {
                duplicate = true;
                break;
            }
        }
        if (!duplicate) {
            kept.append(normalized);
        }
    }
    if (kept.size() > MaxEntries) {
        kept = kept.mid(0, MaxEntries);
    }
    persist(kept);
    return kept;
}

void RecentWorkspaces::remember(const QString &path)
{
    const QString normalized = normalize(path);
    if (normalized.isEmpty() || !QDir(normalized).exists()) {
        return;
    }
    QStringList recents = without(entries(), normalized);
    recents.prepend(normalized);
    if (recents.size() > MaxEntries) {
        recents = recents.mid(0, MaxEntries);
    }
    persist(recents);
    qCDebug(lcCore) << "Remembered workspace" << normalized;
}

void RecentWorkspaces::remove(const QString &path)
{
    persist(without(entries(), path));
}

void RecentWorkspaces::clear()
{
    persist({});
}
