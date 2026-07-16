#include "StartupProfileStore.h"

#include <windows.h>

#include <fstream>
#include <iterator>
#include <stdexcept>

namespace NppThemesShell {

namespace {

constexpr std::uintmax_t maxProfileBytes = 1024U * 1024U;
constexpr char markerText[] = "NppThemes Shell apply in progress\n";

[[nodiscard]] std::string readProfileFile(const std::filesystem::path& path) {
    std::error_code error;
    const auto status = std::filesystem::symlink_status(path, error);
    if (error || !std::filesystem::is_regular_file(status)) {
        throw std::runtime_error("active profile is not a regular file");
    }
    const auto size = std::filesystem::file_size(path, error);
    if (error) {
        throw std::runtime_error("unable to inspect active profile");
    }
    if (size > maxProfileBytes) {
        throw std::invalid_argument("active profile exceeds 1 MiB limit");
    }
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("unable to open active profile");
    }
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

[[nodiscard]] bool writeDurableMarker(const std::filesystem::path& path) noexcept {
    const auto handle = ::CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                                      FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, nullptr);
    if (handle == INVALID_HANDLE_VALUE) {
        return false;
    }
    DWORD written{};
    const auto writeSucceeded = ::WriteFile(handle, markerText, static_cast<DWORD>(sizeof(markerText) - 1U),
                                             &written, nullptr) != FALSE;
    const auto flushSucceeded = writeSucceeded && written == sizeof(markerText) - 1U &&
                                ::FlushFileBuffers(handle) != FALSE;
    ::CloseHandle(handle);
    return flushSucceeded;
}

[[nodiscard]] bool writeDurableFile(const std::filesystem::path& path, const std::string& content) noexcept {
    const auto handle = ::CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                                      FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, nullptr);
    if (handle == INVALID_HANDLE_VALUE || content.size() > MAXDWORD) {
        if (handle != INVALID_HANDLE_VALUE) {
            ::CloseHandle(handle);
        }
        return false;
    }
    DWORD written{};
    const auto writeSucceeded = ::WriteFile(handle, content.data(), static_cast<DWORD>(content.size()), &written,
                                             nullptr) != FALSE;
    const auto flushSucceeded = writeSucceeded && written == content.size() && ::FlushFileBuffers(handle) != FALSE;
    ::CloseHandle(handle);
    return flushSucceeded;
}

} // namespace

StartupProfileStore::StartupProfileStore(std::filesystem::path settingsRoot)
    : _directory(std::move(settingsRoot) / "NppThemes"),
      _profilePath(_directory / "active-profile.json"),
      _markerPath(_directory / "apply.incomplete") {}

StartupProfileLoad StartupProfileStore::load() const {
    std::error_code error;
    if (std::filesystem::exists(_markerPath, error)) {
        if (error || !std::filesystem::remove(_markerPath, error) || error) {
            return {StartupProfileStatus::Rejected, std::nullopt, "unable to clear incomplete-apply marker"};
        }
        return {StartupProfileStatus::RecoveredIncompleteApply, std::nullopt,
                "previous theme apply was incomplete; native rendering retained"};
    }
    if (error) {
        return {StartupProfileStatus::Rejected, std::nullopt, "unable to inspect incomplete-apply marker"};
    }

    if (!std::filesystem::exists(_profilePath, error)) {
        if (error) {
            return {StartupProfileStatus::Rejected, std::nullopt, "unable to inspect active profile"};
        }
        return {};
    }

    try {
        auto profile = nppthemes::deserializeProfile(readProfileFile(_profilePath));
        return {StartupProfileStatus::Ready, std::move(profile), {}};
    } catch (const std::exception& exception) {
        return {StartupProfileStatus::Rejected, std::nullopt, exception.what()};
    }
}

bool StartupProfileStore::beginApply(std::string& error) const {
    std::error_code fileError;
    const auto directoryStatus = std::filesystem::symlink_status(_directory, fileError);
    if (!fileError && std::filesystem::exists(directoryStatus) &&
        (!std::filesystem::is_directory(directoryStatus) || std::filesystem::is_symlink(directoryStatus))) {
        error = "theme settings path is not a regular directory";
        return false;
    }
    fileError.clear();
    std::filesystem::create_directories(_directory, fileError);
    if (fileError) {
        error = "unable to create theme settings directory";
        return false;
    }
    if (std::filesystem::exists(_markerPath, fileError) || fileError) {
        error = "incomplete-apply marker already exists or cannot be inspected";
        return false;
    }

    auto temporary = _markerPath;
    temporary += L".tmp";
    if (!writeDurableMarker(temporary)) {
        error = "unable to durably write incomplete-apply marker";
        return false;
    }
    if (::MoveFileExW(temporary.c_str(), _markerPath.c_str(), MOVEFILE_WRITE_THROUGH) == FALSE) {
        std::error_code ignored;
        std::filesystem::remove(temporary, ignored);
        error = "unable to commit incomplete-apply marker";
        return false;
    }
    error.clear();
    return true;
}

bool StartupProfileStore::persist(const nppthemes::ThemeProfile& profile, std::string& error) const {
    std::error_code fileError;
    if (!std::filesystem::exists(_markerPath, fileError) || fileError) {
        error = "active profile write requires incomplete-apply marker";
        return false;
    }
    const auto profileErrors = nppthemes::validateProfile(profile);
    if (!profileErrors.empty()) {
        error = profileErrors.front();
        return false;
    }
    const auto serialized = nppthemes::serializeProfile(nppthemes::migrateProfileToV2(profile));
    auto temporary = _profilePath;
    temporary += L".tmp";
    if (!writeDurableFile(temporary, serialized)) {
        error = "unable to durably write active profile";
        return false;
    }
    if (::MoveFileExW(temporary.c_str(), _profilePath.c_str(),
                      MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) == FALSE) {
        std::error_code ignored;
        std::filesystem::remove(temporary, ignored);
        error = "unable to atomically replace active profile";
        return false;
    }
    error.clear();
    return true;
}

bool StartupProfileStore::disable(std::string& error) const {
    std::error_code fileError;
    if (!std::filesystem::exists(_markerPath, fileError) || fileError) {
        error = "active profile removal requires incomplete-apply marker";
        return false;
    }
    fileError.clear();
    if (std::filesystem::exists(_profilePath, fileError) &&
        (!std::filesystem::remove(_profilePath, fileError) || fileError)) {
        error = "unable to remove active profile";
        return false;
    }
    if (fileError) {
        error = "unable to inspect active profile for removal";
        return false;
    }
    error.clear();
    return true;
}

bool StartupProfileStore::completeApply(std::string& error) const {
    std::error_code fileError;
    if (!std::filesystem::remove(_markerPath, fileError) || fileError) {
        error = "unable to remove incomplete-apply marker";
        return false;
    }
    error.clear();
    return true;
}

} // namespace NppThemesShell
