#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include "NppThemes/StartupProfileStore.h"
#include "NppThemes/ThemeRuntime.h"

namespace {

void require(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}

class TemporaryDirectory {
public:
    TemporaryDirectory()
        : path(std::filesystem::temp_directory_path() /
               ("NppThemesStartupTests-" + std::to_string(std::rand()))) {
        std::filesystem::remove_all(path);
        std::filesystem::create_directories(path);
    }

    ~TemporaryDirectory() {
        std::error_code ignored;
        std::filesystem::remove_all(path, ignored);
    }

    std::filesystem::path path;
};

void writeFile(const std::filesystem::path& path, const std::string& content) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    output << content;
    require(static_cast<bool>(output), "test fixture write succeeds");
}

class RecordingHost final : public NppThemesShell::DarkModePaletteHost {
public:
    [[nodiscard]] NppThemesShell::DarkModePaletteState capture() const noexcept override {
        return current;
    }

    void apply(const NppThemesShell::DarkModePaletteState& palette) noexcept override {
        current = palette;
        ++applyCount;
    }

    [[nodiscard]] bool rendererDark() const noexcept override { return dark; }
    void setRendererDark(const bool value) noexcept override {
        dark = value;
        ++rendererApplyCount;
    }

    NppThemesShell::DarkModePaletteState current{6, 0x102030U, 0x405060U};
    int applyCount{};
    int rendererApplyCount{};
    bool dark{};
};

} // namespace

int main() {
    TemporaryDirectory temporary;
    NppThemesShell::StartupProfileStore store(temporary.path);

    const auto missing = store.load();
    require(missing.status == NppThemesShell::StartupProfileStatus::Disabled,
            "missing profile keeps native mode");
    require(!std::filesystem::exists(store.directory()), "missing profile performs no write");

    writeFile(store.profilePath(), "{invalid");
    const auto invalid = store.load();
    require(invalid.status == NppThemesShell::StartupProfileStatus::Rejected, "invalid profile rejected");
    require(!invalid.diagnostic.empty(), "invalid profile reports diagnostic");

    const auto profile = nppthemes::builtInProfiles().front();
    writeFile(store.profilePath(), nppthemes::serializeProfile(profile));
    std::string error;
    require(store.beginApply(error), "apply marker begins atomically");
    require(std::filesystem::exists(store.markerPath()), "apply marker exists before mutation");

    const auto recovered = store.load();
    require(recovered.status == NppThemesShell::StartupProfileStatus::RecoveredIncompleteApply,
            "incomplete apply forces one native launch");
    require(!std::filesystem::exists(store.markerPath()), "recovery clears marker");
    require(store.load().status == NppThemesShell::StartupProfileStatus::Ready,
            "profile becomes eligible after safe recovery launch");

    require(store.beginApply(error), "second apply marker begins");
    require(store.completeApply(error), "successful apply clears marker");
    require(!std::filesystem::exists(store.markerPath()), "completed apply leaves no marker");
    require(!store.persist(profile, error), "profile write without marker is rejected");
    require(!store.disable(error), "profile removal without marker is rejected");
    require(std::filesystem::exists(store.profilePath()), "rejected removal preserves active profile");

    RecordingHost host;
    const auto native = host.current;
    NppThemesShell::ThemeRuntime runtime(host);
    const auto activated = runtime.initialize(temporary.path, false);
    require(activated.status == NppThemesShell::StartupProfileStatus::Ready, "valid startup profile activates");
    require(runtime.isActive(), "runtime reports active adapter");
    require(host.current.tone == 32, "runtime applies customized host tone");
    require(host.dark == profile.dark, "startup coordinates host renderer with profile mode");
    require(!std::filesystem::exists(store.markerPath()), "activation commits marker cleanup");
    runtime.setHighContrastActive(true);
    require(host.current == native, "High Contrast restores exact native state");
    require(!host.dark, "High Contrast disables custom dark renderer");
    runtime.setHighContrastActive(false);
    require(host.current.tone == 32, "custom palette resumes after High Contrast");
    require(host.dark == profile.dark, "profile renderer mode resumes after High Contrast");

    auto invalidProfile = profile;
    invalidProfile.fontSizePt = 2;
    require(!runtime.selectProfile(invalidProfile, error), "invalid runtime selection is rejected");
    require(runtime.activeProfile() && runtime.activeProfile()->id == profile.id,
            "invalid selection preserves active profile");
    const auto persistedAfterRejection = store.load();
    require(persistedAfterRejection.profile && persistedAfterRejection.profile->id == profile.id,
            "invalid selection preserves persisted profile");

    const auto profiles = nppthemes::builtInProfiles();
    const auto lightProfile = *std::ranges::find_if(profiles, [](const auto& candidate) { return !candidate.dark; });
    require(runtime.selectProfile(lightProfile, error), "runtime switches to light profile");
    require(!host.dark, "light profile disables host dark renderer");
    require(runtime.activeProfile() && runtime.activeProfile()->id == lightProfile.id,
            "selected profile becomes active");
    const auto persistedLight = store.load();
    require(persistedLight.profile && persistedLight.profile->id == lightProfile.id,
            "selected profile persists atomically");

    const auto darkProfile = *std::ranges::find_if(profiles, [](const auto& candidate) { return candidate.dark; });
    require(runtime.selectProfile(darkProfile, error), "runtime switches to dark profile");
    require(host.dark, "dark profile enables host dark renderer");
    require(runtime.disable(error), "runtime disable succeeds");
    require(!runtime.isActive(), "disable deactivates adapter");
    require(!host.dark, "disable restores original renderer mode");
    require(!std::filesystem::exists(store.profilePath()), "disable removes persisted profile");

    require(runtime.selectProfile(darkProfile, error), "selection works again after disable");
    runtime.shutdown();
    require(host.current == native, "runtime shutdown restores native state");
    require(!host.dark, "runtime shutdown restores original renderer mode");

    TemporaryDirectory oversizedRoot;
    NppThemesShell::StartupProfileStore oversizedStore(oversizedRoot.path);
    std::filesystem::create_directories(oversizedStore.profilePath().parent_path());
    {
        std::ofstream output(oversizedStore.profilePath(), std::ios::binary | std::ios::trunc);
        output.seekp((1024 * 1024));
        output.put('x');
    }
    require(oversizedStore.load().status == NppThemesShell::StartupProfileStatus::Rejected,
            "oversized profile rejected before parsing");

    return 0;
}
