#include "core/SessionStore.h"

#include "core/Logging.h"

#include <QSettings>

namespace {

constexpr auto kWorkspace = "session/workspace";
constexpr auto kOpenFiles = "session/openFiles";
constexpr auto kActiveFile = "session/activeFile";
constexpr auto kGeometry = "session/geometry";
constexpr auto kWindowState = "session/windowState";

} // namespace

SessionState SessionStore::load() const
{
    QSettings settings;
    return load(settings);
}

void SessionStore::save(const SessionState &state)
{
    QSettings settings;
    save(state, settings);
}

SessionState SessionStore::load(QSettings &settings) const
{
    SessionState state;
    state.workspace = settings.value(kWorkspace).toString();
    state.openFiles = settings.value(kOpenFiles).toStringList();
    state.activeFile = settings.value(kActiveFile).toString();
    state.geometry = settings.value(kGeometry).toByteArray();
    state.windowState = settings.value(kWindowState).toByteArray();
    return state;
}

void SessionStore::save(const SessionState &state, QSettings &settings)
{
    settings.setValue(kWorkspace, state.workspace);
    settings.setValue(kOpenFiles, state.openFiles);
    settings.setValue(kActiveFile, state.activeFile);
    settings.setValue(kGeometry, state.geometry);
    settings.setValue(kWindowState, state.windowState);
    qCDebug(lcCore) << "Session saved" << state.workspace << "files" << state.openFiles.size();
}
