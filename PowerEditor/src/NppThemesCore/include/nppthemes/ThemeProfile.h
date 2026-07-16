#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace nppthemes {

using Color = std::uint32_t;

struct Palette {
    Color background{};
    Color surface{};
    Color foreground{};
    Color muted{};
    Color accent{};
    Color keyword{};
    Color string{};
    Color number{};
    Color comment{};
    Color function{};
    Color type{};
    Color op{};
    Color selection{};
    Color caret{};
    Color currentLine{};
    Color whitespace{};

    bool operator==(const Palette&) const = default;
};

enum class ShellDensity {
    Compact,
    Comfortable,
    Touch,
};

struct ShellSettings {
    ShellDensity density{ShellDensity::Comfortable};
    std::string fontFamily{"Segoe UI"};
    int fontSizePt{9};
    std::map<std::string, Color, std::less<>> colorOverrides;

    bool operator==(const ShellSettings&) const = default;
};

struct ThemeProfile {
    int schemaVersion{1};
    std::string id;
    std::string name;
    bool dark{true};
    Palette palette;
    std::string fontFamily{"Cascadia Mono"};
    int fontSizePt{11};
    std::optional<ShellSettings> shell;

    bool operator==(const ThemeProfile&) const = default;
};

[[nodiscard]] std::vector<ThemeProfile> builtInProfiles();
[[nodiscard]] std::optional<Color> parseHexColor(std::string_view value);
[[nodiscard]] std::string formatHexColor(Color value);
[[nodiscard]] double contrastRatio(Color foreground, Color background);
[[nodiscard]] std::vector<std::string> validateProfile(const ThemeProfile& profile);
[[nodiscard]] ThemeProfile migrateProfileToV2(const ThemeProfile& profile);
[[nodiscard]] std::string serializeProfile(const ThemeProfile& profile);
[[nodiscard]] ThemeProfile deserializeProfile(std::string_view jsonText);
[[nodiscard]] Color semanticColorForStyle(const ThemeProfile& profile, std::string_view language, int styleId,
                                          Color existingForeground);

} // namespace nppthemes
