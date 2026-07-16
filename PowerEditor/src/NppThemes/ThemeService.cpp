#include "ThemeService.h"

#include <algorithm>
#include <utility>

namespace NppThemesShell {

bool ThemeService::initialize(const nppthemes::ThemeProfile& profile, const bool highContrastActive,
                              std::string& error) {
    ThemeSnapshot resolved;
    if (!resolve(profile, ThemeRenderMode::Custom, resolved, error)) {
        return false;
    }
    _highContrastActive = highContrastActive;
    resolved.renderMode = effectiveMode(ThemeRenderMode::Custom);
    resolved.generation = _active ? _active->generation + 1 : 1;
    std::optional<ThemeSnapshot> candidate{std::move(resolved)};
    _active.swap(candidate);
    _previewOrigin.reset();
    publish();
    return true;
}

bool ThemeService::apply(const nppthemes::ThemeProfile& profile, std::string& error) {
    if (!_active) {
        error = "ThemeService is not initialized";
        return false;
    }
    ThemeSnapshot resolved;
    if (!resolve(profile, ThemeRenderMode::Custom, resolved, error)) {
        return false;
    }
    resolved.generation = _active->generation + 1;
    std::optional<ThemeSnapshot> candidate{std::move(resolved)};
    _active.swap(candidate);
    _previewOrigin.reset();
    publish();
    return true;
}

bool ThemeService::beginPreview(const nppthemes::ThemeProfile& profile, std::string& error) {
    if (!_active) {
        error = "ThemeService is not initialized";
        return false;
    }
    ThemeSnapshot resolved;
    if (!resolve(profile, ThemeRenderMode::Preview, resolved, error)) {
        return false;
    }
    resolved.generation = _active->generation + 1;

    if (!_previewOrigin) {
        _previewOrigin = _active;
    }
    std::optional<ThemeSnapshot> candidate{std::move(resolved)};
    _active.swap(candidate);
    publish();
    return true;
}

bool ThemeService::commitPreview() noexcept {
    if (!_active || !_previewOrigin) {
        return false;
    }
    _previewOrigin.reset();
    _active->renderMode = effectiveMode(ThemeRenderMode::Custom);
    ++_active->generation;
    publish();
    return true;
}

bool ThemeService::cancelPreview() noexcept {
    if (!_active || !_previewOrigin) {
        return false;
    }
    const auto generation = _active->generation + 1;
    _active.swap(_previewOrigin);
    _previewOrigin.reset();
    _active->renderMode = effectiveMode(ThemeRenderMode::Custom);
    _active->generation = generation;
    publish();
    return true;
}

void ThemeService::setHighContrastActive(const bool active) noexcept {
    if (_highContrastActive == active) {
        return;
    }
    _highContrastActive = active;
    if (!_active) {
        return;
    }
    _active->renderMode = effectiveMode(_previewOrigin ? ThemeRenderMode::Preview : ThemeRenderMode::Custom);
    ++_active->generation;
    publish();
}

bool ThemeService::subscribe(ThemeSubscriber& subscriber) {
    if (std::ranges::find(_subscribers, &subscriber) != _subscribers.end()) {
        return false;
    }
    _subscribers.push_back(&subscriber);
    return true;
}

void ThemeService::unsubscribe(ThemeSubscriber& subscriber) noexcept {
    std::erase(_subscribers, &subscriber);
}

const ThemeSnapshot* ThemeService::snapshot() const noexcept {
    return _active ? &*_active : nullptr;
}

bool ThemeService::resolve(const nppthemes::ThemeProfile& profile, const ThemeRenderMode requestedMode,
                           ThemeSnapshot& resolved, std::string& error) const {
    const auto profileErrors = nppthemes::validateProfile(profile);
    if (!profileErrors.empty()) {
        error = profileErrors.front();
        return false;
    }

    auto migrated = nppthemes::migrateProfileToV2(profile);
    auto palette = nppthemes::deriveShellPalette(migrated);
    const auto paletteErrors = nppthemes::validateShellPalette(palette);
    if (!paletteErrors.empty()) {
        error = paletteErrors.front();
        return false;
    }

    resolved.profile = std::move(migrated);
    resolved.palette = palette;
    resolved.renderMode = effectiveMode(requestedMode);
    error.clear();
    return true;
}

ThemeRenderMode ThemeService::effectiveMode(const ThemeRenderMode requestedMode) const noexcept {
    return _highContrastActive ? ThemeRenderMode::Native : requestedMode;
}

void ThemeService::publish() noexcept {
    if (!_active) {
        return;
    }
    for (auto* subscriber : _subscribers) {
        subscriber->onThemeChanged(*_active);
    }
}

} // namespace NppThemesShell
