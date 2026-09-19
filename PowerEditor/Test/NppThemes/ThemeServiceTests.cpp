#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <utility>

#include "NppThemes/AppSurfaceTheme.h"
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

    [[nodiscard]] bool rendererDark() const noexcept override { return dark; }
    void setRendererDark(const bool value) noexcept override { dark = value; }

    mutable int captureCount{};
    int applyCount{};
    NppThemesShell::DarkModePaletteState current{7, 0x010203U, 0x040506U};
    bool dark{};
};

class RecordingSurfaceHost final : public NppThemesShell::AppSurfaceThemeHost {
public:
    [[nodiscard]] std::optional<NppThemesShell::AppSurfaceThemeState> capture() const override {
        ++captureCount;
        return current;
    }

    void apply(std::optional<NppThemesShell::AppSurfaceThemeState> state) override {
        ++applyCount;
        current = std::move(state);
    }

    mutable int captureCount{};
    int applyCount{};
    std::optional<NppThemesShell::AppSurfaceThemeState> current;
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

    RecordingSurfaceHost surfaceHost;
    NppThemesShell::AppSurfaceThemeAdapter surfaceAdapter(service, surfaceHost);
    require(surfaceAdapter.activate(), "app-surface adapter activates");
    require(surfaceHost.captureCount == 1, "app-surface adapter captures prior state once");
    require(surfaceHost.current && surfaceHost.current->profile.id == service.snapshot()->profile.id,
            "app-surface adapter publishes active profile");
    require(surfaceHost.current && surfaceHost.current->palette.tabActive == service.snapshot()->palette.tabActive,
            "app-surface adapter publishes resolved tokens");
    require(NppThemesShell::appSurfaceRoleColor(surfaceHost.current->palette,
                                                NppThemesShell::AppSurfaceRole::ToolbarHover) ==
                surfaceHost.current->palette.toolbarHover,
            "app-surface role mapping preserves dedicated toolbar token");
    require(NppThemesShell::appSurfaceRoleColor(surfaceHost.current->palette,
                                                NppThemesShell::AppSurfaceRole::DialogSurface) ==
                surfaceHost.current->palette.dialogSurface,
            "app-surface role mapping preserves dedicated dialog token");

    auto& productionSurfaceHost = NppThemesShell::appSurfaceThemeHost();
    const auto originalSurfaceState = productionSurfaceHost.capture();
    productionSurfaceHost.apply(*surfaceHost.current);
    require(NppThemesShell::activeAppSurfaceColor(NppThemesShell::AppSurfaceRole::MenuBackground) != CLR_INVALID,
            "active surface exposes popup color");
    require(NppThemesShell::activeAppSurfaceBrush(NppThemesShell::AppSurfaceRole::ToolbarBackground) != nullptr,
            "active surface caches toolbar brush");
    require(NppThemesShell::activeAppSurfacePen(NppThemesShell::AppSurfaceRole::Divider) != nullptr,
            "active surface caches docking divider pen");
    productionSurfaceHost.apply(std::nullopt);
    require(NppThemesShell::activeAppSurfaceColorOr(NppThemesShell::AppSurfaceRole::ControlBackground,
                                                     RGB(1, 2, 3)) == RGB(1, 2, 3),
            "inactive surface returns caller fallback for panel controls");
    productionSurfaceHost.apply(originalSurfaceState);

    service.setHighContrastActive(true);
    require(!surfaceHost.current, "High Contrast clears custom app-surface state");
    service.setHighContrastActive(false);
    require(surfaceHost.captureCount == 2, "app-surface adapter recaptures after native fallback");
    require(surfaceHost.current.has_value(), "app-surface state resumes after High Contrast");

    surfaceAdapter.deactivate();
    require(!surfaceHost.current, "app-surface adapter restores prior native state");
    require(!surfaceAdapter.isActive(), "app-surface adapter reports inactive after deactivation");

    return 0;
}
