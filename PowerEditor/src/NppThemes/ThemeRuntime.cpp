#include "ThemeRuntime.h"

#include <utility>

namespace NppThemesShell {

ThemeRuntime::ThemeRuntime(DarkModePaletteHost& host) noexcept : _host(host) {}

ThemeRuntime::~ThemeRuntime() {
    shutdown();
}

ThemeRuntimeResult ThemeRuntime::initialize(const std::filesystem::path& settingsRoot,
                                            const bool highContrastActive) {
    if (_adapter) {
        return {StartupProfileStatus::Rejected, "theme runtime already initialized"};
    }

    const StartupProfileStore store(settingsRoot);
    auto loaded = store.load();
    if (loaded.status != StartupProfileStatus::Ready || !loaded.profile) {
        return {loaded.status, std::move(loaded.diagnostic)};
    }

    std::string error;
    if (!store.beginApply(error)) {
        return {StartupProfileStatus::Rejected, std::move(error)};
    }
    if (!_service.initialize(*loaded.profile, highContrastActive, error)) {
        return {StartupProfileStatus::Rejected, std::move(error)};
    }

    auto adapter = std::make_unique<DarkModeThemeAdapter>(_service, _host);
    if (!adapter->activate()) {
        return {StartupProfileStatus::Rejected, "unable to activate host palette adapter"};
    }
    if (!store.completeApply(error)) {
        adapter->deactivate();
        return {StartupProfileStatus::Rejected, std::move(error)};
    }

    _adapter = std::move(adapter);
    return {StartupProfileStatus::Ready, {}};
}

void ThemeRuntime::setHighContrastActive(const bool active) noexcept {
    if (_service.isInitialized()) {
        _service.setHighContrastActive(active);
    }
}

void ThemeRuntime::shutdown() noexcept {
    if (_adapter) {
        _adapter->deactivate();
        _adapter.reset();
    }
}

bool ThemeRuntime::isActive() const noexcept {
    return _adapter && _adapter->isActive();
}

} // namespace NppThemesShell
