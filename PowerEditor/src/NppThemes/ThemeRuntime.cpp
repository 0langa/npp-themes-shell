#include "ThemeRuntime.h"

#include <utility>

namespace NppThemesShell {

ThemeRuntime::ThemeRuntime(DarkModePaletteHost& host, AppSurfaceThemeHost& surfaceHost) noexcept
    : _host(host), _surfaceHost(surfaceHost) {}

ThemeRuntime::~ThemeRuntime() {
    shutdown();
}

ThemeRuntimeResult ThemeRuntime::initialize(const std::filesystem::path& settingsRoot,
                                            const bool highContrastActive) {
    if (_store) {
        return {StartupProfileStatus::Rejected, "theme runtime already initialized"};
    }

    _highContrastActive = highContrastActive;
    _store = std::make_unique<StartupProfileStore>(settingsRoot);
    auto loaded = _store->load();
    if (loaded.status != StartupProfileStatus::Ready || !loaded.profile) {
        return {loaded.status, std::move(loaded.diagnostic)};
    }

    std::string error;
    if (!_store->beginApply(error)) {
        return {StartupProfileStatus::Rejected, std::move(error)};
    }
    if (!_service.initialize(*loaded.profile, highContrastActive, error)) {
        return {StartupProfileStatus::Rejected, std::move(error)};
    }

    _originalRendererDark = _host.rendererDark();
    auto adapter = std::make_unique<DarkModeThemeAdapter>(_service, _host);
    if (!adapter->activate()) {
        return {StartupProfileStatus::Rejected, "unable to activate host palette adapter"};
    }
    auto surfaceAdapter = std::make_unique<AppSurfaceThemeAdapter>(_service, _surfaceHost);
    if (!surfaceAdapter->activate()) {
        adapter->deactivate();
        return {StartupProfileStatus::Rejected, "unable to activate app-surface adapter"};
    }
    _host.setRendererDark(loaded.profile->dark && !highContrastActive);
    if (!_store->completeApply(error)) {
        surfaceAdapter->deactivate();
        adapter->deactivate();
        _host.setRendererDark(*_originalRendererDark);
        _originalRendererDark.reset();
        return {StartupProfileStatus::Rejected, std::move(error)};
    }

    _adapter = std::move(adapter);
    _surfaceAdapter = std::move(surfaceAdapter);
    _activeProfile = nppthemes::migrateProfileToV2(*loaded.profile);
    return {StartupProfileStatus::Ready, {}};
}

bool ThemeRuntime::selectProfile(const nppthemes::ThemeProfile& profile, std::string& error) {
    if (!_store) {
        error = "theme runtime is not initialized";
        return false;
    }
    const auto profileErrors = nppthemes::validateProfile(profile);
    if (!profileErrors.empty()) {
        error = profileErrors.front();
        return false;
    }
    auto migrated = nppthemes::migrateProfileToV2(profile);
    const auto paletteErrors = nppthemes::validateShellPalette(nppthemes::deriveShellPalette(migrated));
    if (!paletteErrors.empty()) {
        error = paletteErrors.front();
        return false;
    }
    if (!_store->beginApply(error) || !_store->persist(migrated, error)) {
        return false;
    }

    if (_service.isInitialized()) {
        if (!_service.apply(migrated, error)) {
            return false;
        }
    } else if (!_service.initialize(migrated, _highContrastActive, error)) {
        return false;
    }

    if (!_originalRendererDark) {
        _originalRendererDark = _host.rendererDark();
    }
    if (!_adapter) {
        auto adapter = std::make_unique<DarkModeThemeAdapter>(_service, _host);
        if (!adapter->activate()) {
            error = "unable to activate host palette adapter";
            return false;
        }
        _adapter = std::move(adapter);
    }
    if (!_surfaceAdapter) {
        auto surfaceAdapter = std::make_unique<AppSurfaceThemeAdapter>(_service, _surfaceHost);
        if (!surfaceAdapter->activate()) {
            error = "unable to activate app-surface adapter";
            return false;
        }
        _surfaceAdapter = std::move(surfaceAdapter);
    }
    _host.setRendererDark(migrated.dark && !_highContrastActive);
    _activeProfile = std::move(migrated);

    if (!_store->completeApply(error)) {
        shutdown();
        return false;
    }
    error.clear();
    return true;
}

bool ThemeRuntime::disable(std::string& error) {
    if (!_store) {
        error = "theme runtime is not initialized";
        return false;
    }
    if (!_store->beginApply(error) || !_store->disable(error)) {
        return false;
    }
    shutdown();
    if (!_store->completeApply(error)) {
        return false;
    }
    error.clear();
    return true;
}

void ThemeRuntime::setHighContrastActive(const bool active) noexcept {
    _highContrastActive = active;
    if (_service.isInitialized()) {
        _service.setHighContrastActive(active);
    }
    if (_activeProfile) {
        _host.setRendererDark(_activeProfile->dark && !active);
    }
}

void ThemeRuntime::shutdown() noexcept {
    if (_surfaceAdapter) {
        _surfaceAdapter->deactivate();
        _surfaceAdapter.reset();
    }
    if (_adapter) {
        _adapter->deactivate();
        _adapter.reset();
    }
    if (_originalRendererDark) {
        _host.setRendererDark(*_originalRendererDark);
        _originalRendererDark.reset();
    }
    _activeProfile.reset();
}

bool ThemeRuntime::isActive() const noexcept {
    return _adapter && _adapter->isActive() && _surfaceAdapter && _surfaceAdapter->isActive();
}

const nppthemes::ThemeProfile* ThemeRuntime::activeProfile() const noexcept {
    return _activeProfile ? &*_activeProfile : nullptr;
}

} // namespace NppThemesShell
