#pragma once

#include <filesystem>
#include <string>

#include "nppthemes/ThemeProfile.h"

namespace nppthemes {

struct ThemeCompileResult {
    bool success{false};
    std::size_t stylesUpdated{0};
    std::string error;
};

[[nodiscard]] ThemeCompileResult compileThemeXml(const std::filesystem::path& source,
                                                 const std::filesystem::path& destination, const ThemeProfile& profile);

} // namespace nppthemes
