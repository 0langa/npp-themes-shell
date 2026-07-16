#pragma once

#include <filesystem>
#include <optional>
#include <string>

#include "nppthemes/ThemeProfile.h"

namespace NppThemesShell {

enum class StartupProfileStatus {
    Disabled,
    Ready,
    RecoveredIncompleteApply,
    Rejected,
};

struct StartupProfileLoad {
    StartupProfileStatus status{StartupProfileStatus::Disabled};
    std::optional<nppthemes::ThemeProfile> profile;
    std::string diagnostic;
};

class StartupProfileStore {
public:
    explicit StartupProfileStore(std::filesystem::path settingsRoot);

    [[nodiscard]] StartupProfileLoad load() const;
    [[nodiscard]] bool beginApply(std::string& error) const;
    [[nodiscard]] bool persist(const nppthemes::ThemeProfile& profile, std::string& error) const;
    [[nodiscard]] bool disable(std::string& error) const;
    [[nodiscard]] bool completeApply(std::string& error) const;

    [[nodiscard]] const std::filesystem::path& directory() const noexcept { return _directory; }
    [[nodiscard]] const std::filesystem::path& profilePath() const noexcept { return _profilePath; }
    [[nodiscard]] const std::filesystem::path& markerPath() const noexcept { return _markerPath; }

private:
    std::filesystem::path _directory;
    std::filesystem::path _profilePath;
    std::filesystem::path _markerPath;
};

} // namespace NppThemesShell
