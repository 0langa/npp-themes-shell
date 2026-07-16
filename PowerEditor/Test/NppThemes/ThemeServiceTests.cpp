#include <cstdlib>
#include <iostream>
#include <string>

#include "NppThemes/DarkModeThemeAdapter.h"
#include "NppThemes/ThemeService.h"

namespace {

void require(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}

class RecordingSubscriber final : public NppThemesShell::ThemeSubscriber {
public:
    void onThemeChanged(const NppThemesShell::ThemeSnapshot& snapshot) noexcept override {
        ++count;
        lastGeneration = snapshot.generation;
        lastMode = snapshot.renderMode;
    }

    int count{};
    std::uint64_t lastGeneration{};
    NppThemesShell::ThemeRenderMode lastMode{NppThemesShell::ThemeRenderMode::Native};
};

class RecordingDarkModeHost final : public NppThemesShell::DarkModePaletteHost {
public:
    [[nodiscard]] NppThemesShell::DarkModePaletteState capture() const noexcept override {
        ++captureCount;
        return current;
    }

    void apply(const NppThemesShell::DarkModePaletteState& palette) noexcept override {
        ++applyCount;
        current = palette;
    }

    mutable int captureCount{};
    int applyCount{};
    NppThemesShell::DarkModePaletteState current{7, 0x010203U, 0x040506U};
};

} // namespace

int main() {
    const auto profiles = nppthemes::builtInProfiles();
    require(profiles.size() >= 2, "built-in profiles available");

    NppThemesShell::ThemeService service;
    RecordingSubscriber subscriber;
    require(service.subscribe(subscriber), "first subscription accepted");
    require(!service.subscribe(subscriber), "duplicate subscription rejected");

    std::string error;
    require(service.initialize(profiles[0], false, error), "service initializes");
    require(service.snapshot() != nullptr, "initialized snapshot exists");
    require(service.snapshot()->renderMode == NppThemesShell::ThemeRenderMode::Custom,
            "normal initialization enables custom rendering");
    require(subscriber.count == 1, "initialization publishes once");

    const auto originalId = service.snapshot()->profile.id;
    require(service.beginPreview(profiles[1], error), "preview starts");
    require(service.isPreviewing(), "preview state recorded");
    require(service.snapshot()->profile.id == profiles[1].id, "preview profile active");
    require(service.snapshot()->renderMode == NppThemesShell::ThemeRenderMode::Preview, "preview mode published");
    require(service.cancelPreview(), "preview cancels");
    require(!service.isPreviewing(), "preview state cleared after cancel");
    require(service.snapshot()->profile.id == originalId, "cancel restores exact profile");

    require(service.beginPreview(profiles[1], error), "second preview starts");
    require(service.commitPreview(), "preview commits");
    require(service.snapshot()->profile.id == profiles[1].id, "committed preview remains active");
    require(service.snapshot()->renderMode == NppThemesShell::ThemeRenderMode::Custom,
            "commit leaves custom mode");

    const auto committed = *service.snapshot();
    auto invalid = profiles[0];
    invalid.fontSizePt = 2;
    require(!service.apply(invalid, error), "invalid profile rejected");
    require(!error.empty(), "invalid profile reports reason");
    require(service.snapshot()->profile == committed.profile, "rejected apply preserves profile");
    require(service.snapshot()->palette == committed.palette, "rejected apply preserves tokens");
    require(service.snapshot()->generation == committed.generation, "rejected apply preserves generation");

    service.setHighContrastActive(true);
    require(service.snapshot()->renderMode == NppThemesShell::ThemeRenderMode::Native,
            "High Contrast forces native rendering");
    require(service.apply(profiles[0], error), "profile can update under High Contrast");
    require(service.snapshot()->renderMode == NppThemesShell::ThemeRenderMode::Native,
            "profile cannot override High Contrast");
    service.setHighContrastActive(false);
    require(service.snapshot()->renderMode == NppThemesShell::ThemeRenderMode::Custom,
            "custom rendering resumes after High Contrast");

    const auto countBeforeUnsubscribe = subscriber.count;
    service.unsubscribe(subscriber);
    require(service.apply(profiles[1], error), "apply succeeds after unsubscribe");
    require(subscriber.count == countBeforeUnsubscribe, "unsubscribed surface receives no event");
    require(!service.commitPreview(), "commit without preview is a no-op");
    require(!service.cancelPreview(), "cancel without preview is a no-op");

    RecordingDarkModeHost host;
    const auto nativePalette = host.current;
    NppThemesShell::DarkModeThemeAdapter adapter(service, host);
    require(adapter.activate(), "dark-mode adapter activates");
    require(!adapter.activate(), "dark-mode adapter rejects duplicate activation");
    require(host.captureCount == 1, "adapter captures native palette once");
    require(host.current.tone == 32, "adapter selects customized host tone");
    require(host.current.background == service.snapshot()->palette.windowBackground,
            "adapter maps window background");
    require(host.current.controlBackground == service.snapshot()->palette.controlBackground,
            "adapter maps control background");

    service.setHighContrastActive(true);
    require(host.current == nativePalette, "High Contrast restores exact native host palette");
    service.setHighContrastActive(false);
    require(host.captureCount == 2, "adapter recaptures after native fallback");
    require(host.current.tone == 32, "adapter resumes customized tone after High Contrast");

    adapter.deactivate();
    require(host.current == nativePalette, "adapter deactivation restores native host palette");
    require(!adapter.isActive(), "adapter reports inactive after deactivation");

    return 0;
}
