#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "nppthemes/ShellPalette.h"
#include "nppthemes/ThemeProfile.h"

namespace NppThemesShell {

enum class ThemeRenderMode {
    Native,
    Custom,
    Preview,
};

struct ThemeSnapshot {
    nppthemes::ThemeProfile profile;
    nppthemes::ShellPalette palette;
    ThemeRenderMode renderMode{ThemeRenderMode::Native};
    std::uint64_t generation{};
};

class ThemeSubscriber {
public:
    virtual ~ThemeSubscriber() = default;
    virtual void onThemeChanged(const ThemeSnapshot& snapshot) noexcept = 0;
};

// Main-UI-thread service. Profiles are fully resolved before active state changes.
class ThemeService {
public:
    [[nodiscard]] bool initialize(const nppthemes::ThemeProfile& profile, bool highContrastActive,
                                  std::string& error);
    [[nodiscard]] bool apply(const nppthemes::ThemeProfile& profile, std::string& error);
    [[nodiscard]] bool beginPreview(const nppthemes::ThemeProfile& profile, std::string& error);
    [[nodiscard]] bool commitPreview() noexcept;
    [[nodiscard]] bool cancelPreview() noexcept;

    void setHighContrastActive(bool active) noexcept;

    [[nodiscard]] bool subscribe(ThemeSubscriber& subscriber);
    void unsubscribe(ThemeSubscriber& subscriber) noexcept;

    [[nodiscard]] bool isInitialized() const noexcept { return _active.has_value(); }
    [[nodiscard]] bool isPreviewing() const noexcept { return _previewOrigin.has_value(); }
    [[nodiscard]] bool isHighContrastActive() const noexcept { return _highContrastActive; }
    [[nodiscard]] const ThemeSnapshot* snapshot() const noexcept;

private:
    [[nodiscard]] bool resolve(const nppthemes::ThemeProfile& profile, ThemeRenderMode requestedMode,
                               ThemeSnapshot& resolved, std::string& error) const;
    [[nodiscard]] ThemeRenderMode effectiveMode(ThemeRenderMode requestedMode) const noexcept;
    void publish() noexcept;

    std::optional<ThemeSnapshot> _active;
    std::optional<ThemeSnapshot> _previewOrigin;
    std::vector<ThemeSubscriber*> _subscribers;
    bool _highContrastActive{};
};

} // namespace NppThemesShell
