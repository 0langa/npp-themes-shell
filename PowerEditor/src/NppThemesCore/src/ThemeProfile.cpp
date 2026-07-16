#include "nppthemes/ThemeProfile.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

#if __has_include(<nlohmann/json.hpp>)
#include <nlohmann/json.hpp>
#else
#include <json/json.hpp>
#endif

#include "nppthemes/ShellPalette.h"

namespace nppthemes {
namespace {

using nlohmann::json;

enum class Role {
    Foreground,
    Muted,
    Accent,
    Keyword,
    String,
    Number,
    Comment,
    Function,
    Type,
    Operator,
};

[[nodiscard]] std::string lower(std::string_view input) {
    std::string result(input);
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return result;
}

[[nodiscard]] bool hasControlCharacters(std::string_view value) {
    return std::any_of(value.begin(), value.end(),
                       [](unsigned char character) { return character < 0x20U || character == 0x7FU; });
}

[[nodiscard]] bool validIdentifier(std::string_view value) {
    return !value.empty() && std::all_of(value.begin(), value.end(), [](unsigned char character) {
        return std::isalnum(character) != 0 || character == '.' || character == '_' || character == '-';
    });
}

[[nodiscard]] bool nestingWithinLimit(std::string_view text, std::size_t limit) {
    std::size_t depth = 0;
    bool inString = false;
    bool escaped = false;
    for (const char character : text) {
        if (inString) {
            if (escaped) {
                escaped = false;
            } else if (character == '\\') {
                escaped = true;
            } else if (character == '"') {
                inString = false;
            }
            continue;
        }
        if (character == '"') {
            inString = true;
        } else if (character == '{' || character == '[') {
            ++depth;
            if (depth > limit) {
                return false;
            }
        } else if ((character == '}' || character == ']') && depth > 0) {
            --depth;
        }
    }
    return true;
}

[[nodiscard]] Color colorForRole(const Palette& palette, Role role) {
    switch (role) {
    case Role::Muted:
        return palette.muted;
    case Role::Accent:
        return palette.accent;
    case Role::Keyword:
        return palette.keyword;
    case Role::String:
        return palette.string;
    case Role::Number:
        return palette.number;
    case Role::Comment:
        return palette.comment;
    case Role::Function:
        return palette.function;
    case Role::Type:
        return palette.type;
    case Role::Operator:
        return palette.op;
    case Role::Foreground:
    default:
        return palette.foreground;
    }
}

[[nodiscard]] std::optional<Role> roleForCStyle(int style) {
    switch (style) {
    case 1:
    case 2:
    case 3:
    case 15:
    case 17:
        return Role::Comment;
    case 4:
        return Role::Number;
    case 5:
    case 9:
    case 16:
        return Role::Keyword;
    case 6:
    case 7:
    case 12:
    case 13:
    case 14:
    case 20:
    case 21:
    case 22:
    case 23:
    case 24:
    case 25:
        return Role::String;
    case 8:
    case 19:
        return Role::Type;
    case 10:
        return Role::Operator;
    case 11:
        return Role::Foreground;
    default:
        return std::nullopt;
    }
}

[[nodiscard]] std::optional<Role> roleForPython(int style) {
    switch (style) {
    case 1:
    case 12:
        return Role::Comment;
    case 2:
        return Role::Number;
    case 3:
    case 4:
    case 6:
    case 7:
    case 13:
    case 16:
    case 17:
    case 18:
    case 19:
        return Role::String;
    case 5:
    case 14:
        return Role::Keyword;
    case 8:
        return Role::Type;
    case 9:
    case 15:
        return Role::Function;
    case 10:
        return Role::Operator;
    default:
        return std::nullopt;
    }
}

[[nodiscard]] std::optional<Role> roleForJson(int style) {
    switch (style) {
    case 1:
        return Role::Number;
    case 2:
    case 3:
    case 4:
    case 5:
    case 9:
        return Role::String;
    case 6:
    case 7:
        return Role::Comment;
    case 8:
        return Role::Operator;
    case 11:
    case 12:
        return Role::Keyword;
    case 13:
        return Role::Accent;
    default:
        return std::nullopt;
    }
}

[[nodiscard]] std::optional<Role> roleForMarkup(int style) {
    if (style >= 9 && style <= 12) {
        return Role::String;
    }
    if ((style >= 20 && style <= 22) || style == 29 || style == 30) {
        return Role::Comment;
    }
    switch (style) {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
        return Role::Keyword;
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
    case 18:
    case 19:
        return Role::Operator;
    default:
        return std::nullopt;
    }
}

[[nodiscard]] std::optional<Role> roleForSql(int style) {
    switch (style) {
    case 1:
    case 2:
    case 3:
    case 13:
    case 15:
        return Role::Comment;
    case 4:
        return Role::Number;
    case 5:
    case 16:
    case 17:
    case 18:
        return Role::Keyword;
    case 6:
    case 7:
    case 8:
    case 12:
        return Role::String;
    case 10:
        return Role::Operator;
    case 11:
        return Role::Type;
    default:
        return std::nullopt;
    }
}

[[nodiscard]] std::optional<Role> roleForShell(int style) {
    switch (style) {
    case 1:
    case 2:
        return Role::Comment;
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 12:
    case 13:
        return Role::String;
    case 8:
        return Role::Number;
    case 9:
    case 10:
        return Role::Keyword;
    case 11:
        return Role::Operator;
    case 14:
    case 15:
        return Role::Function;
    default:
        return std::nullopt;
    }
}

[[nodiscard]] std::optional<Role> roleForStyle(std::string_view language, int style) {
    const auto lang = lower(language);
    if (lang.find("python") != std::string::npos) {
        return roleForPython(style);
    }
    if (lang.find("json") != std::string::npos) {
        return roleForJson(style);
    }
    if (lang.find("html") != std::string::npos || lang.find("xml") != std::string::npos ||
        lang.find("php") != std::string::npos) {
        return roleForMarkup(style);
    }
    if (lang.find("sql") != std::string::npos) {
        return roleForSql(style);
    }
    if (lang.find("bash") != std::string::npos || lang.find("shell") != std::string::npos ||
        lang.find("powershell") != std::string::npos) {
        return roleForShell(style);
    }
    if (lang.find("c") != std::string::npos || lang.find("java") != std::string::npos ||
        lang.find("javascript") != std::string::npos || lang.find("typescript") != std::string::npos ||
        lang.find("rust") != std::string::npos || lang.find("go") != std::string::npos ||
        lang.find("swift") != std::string::npos) {
        return roleForCStyle(style);
    }
    return std::nullopt;
}

[[nodiscard]] double channelLuminance(std::uint8_t value) {
    const double channel = static_cast<double>(value) / 255.0;
    return channel <= 0.04045 ? channel / 12.92 : std::pow((channel + 0.055) / 1.055, 2.4);
}

void paletteToJson(json& target, const Palette& palette) {
    target = {
        {"background", formatHexColor(palette.background)},
        {"surface", formatHexColor(palette.surface)},
        {"foreground", formatHexColor(palette.foreground)},
        {"muted", formatHexColor(palette.muted)},
        {"accent", formatHexColor(palette.accent)},
        {"keyword", formatHexColor(palette.keyword)},
        {"string", formatHexColor(palette.string)},
        {"number", formatHexColor(palette.number)},
        {"comment", formatHexColor(palette.comment)},
        {"function", formatHexColor(palette.function)},
        {"type", formatHexColor(palette.type)},
        {"operator", formatHexColor(palette.op)},
        {"selection", formatHexColor(palette.selection)},
        {"caret", formatHexColor(palette.caret)},
        {"currentLine", formatHexColor(palette.currentLine)},
        {"whitespace", formatHexColor(palette.whitespace)},
    };
}

[[nodiscard]] Color requiredColor(const json& source, const char* name) {
    const auto parsed = parseHexColor(source.at(name).get<std::string>());
    if (!parsed) {
        throw std::invalid_argument(std::string("Invalid color field: ") + name);
    }
    return *parsed;
}

[[nodiscard]] Palette paletteFromJson(const json& source) {
    return {
        requiredColor(source, "background"), requiredColor(source, "surface"), requiredColor(source, "foreground"),
        requiredColor(source, "muted"),      requiredColor(source, "accent"),  requiredColor(source, "keyword"),
        requiredColor(source, "string"),     requiredColor(source, "number"),  requiredColor(source, "comment"),
        requiredColor(source, "function"),   requiredColor(source, "type"),    requiredColor(source, "operator"),
        requiredColor(source, "selection"),  requiredColor(source, "caret"),   requiredColor(source, "currentLine"),
        requiredColor(source, "whitespace"),
    };
}

[[nodiscard]] const char* densityName(ShellDensity density) noexcept {
    switch (density) {
    case ShellDensity::Compact:
        return "compact";
    case ShellDensity::Touch:
        return "touch";
    case ShellDensity::Comfortable:
    default:
        return "comfortable";
    }
}

[[nodiscard]] ShellDensity densityFromName(std::string_view value) {
    if (value == "compact") {
        return ShellDensity::Compact;
    }
    if (value == "comfortable") {
        return ShellDensity::Comfortable;
    }
    if (value == "touch") {
        return ShellDensity::Touch;
    }
    throw std::invalid_argument("shell.density must be compact, comfortable, or touch");
}

} // namespace

std::vector<ThemeProfile> builtInProfiles() {
    return {
        {1,
         "builtin.northern-lights",
         "Northern Lights",
         true,
         {0x11151C, 0x1A202B, 0xD8DEE9, 0x7D899A, 0x7DD3FC, 0xC4A7E7, 0xA6DA95, 0xF5A97F, 0x6E8299, 0x8BD5CA, 0x91D7E3,
          0xF0C6C6, 0x294B63, 0xF4F7FB, 0x171E29, 0x445064},
         "Cascadia Mono",
         11},
        {1,
         "builtin.graphite",
         "Graphite",
         true,
         {0x171717, 0x232323, 0xE5E5E5, 0x8B8B8B, 0xA3E635, 0xC4B5FD, 0x86EFAC, 0xFDE68A, 0x737373, 0x67E8F9, 0x93C5FD,
          0xFCA5A5, 0x3F4A2B, 0xFFFFFF, 0x20251B, 0x525252},
         "Cascadia Mono",
         11},
        {1,
         "builtin.midnight-neon",
         "Midnight Neon",
         true,
         {0x090B16, 0x11162A, 0xE6E9FF, 0x747DA1, 0x38BDF8, 0xF472B6, 0x4ADE80, 0xFBBF24, 0x64748B, 0x22D3EE, 0xA78BFA,
          0xFB7185, 0x123453, 0xF8FAFC, 0x10182C, 0x34405F},
         "Cascadia Mono",
         11},
        {1,
         "builtin.paper",
         "Paper",
         false,
         {0xFAFAF8, 0xF0F0EC, 0x242424, 0x6B7280, 0x0369A1, 0x7C3AED, 0x15803D, 0xB45309, 0x64748B, 0x0E7490, 0x1D4ED8,
          0xBE123C, 0xCDE8F6, 0x111827, 0xEEF6FA, 0xA3A3A3},
         "Cascadia Mono",
         11},
        {1,
         "builtin.warm-sand",
         "Warm Sand",
         false,
         {0xFFF9ED, 0xF6EEDC, 0x332B22, 0x776B5D, 0xA34317, 0x7C3F91, 0x3F6E4B, 0xA15C00, 0x7A7064, 0x006D77, 0x3A5A9B,
          0xB42318, 0xF0D8B5, 0x211A14, 0xFBF0DA, 0xAA9D8B},
         "Cascadia Mono",
         11},
        {1,
         "builtin.accessible-dark",
         "Accessible Dark",
         true,
         {0x000000, 0x151515, 0xFFFFFF, 0xB8B8B8, 0x66CCFF, 0xFF99FF, 0x99FF99, 0xFFD27F, 0xB8B8B8, 0x66FFFF, 0x99CCFF,
          0xFF9999, 0x194866, 0xFFFFFF, 0x101820, 0x8A8A8A},
         "Consolas",
         12},
    };
}

std::optional<Color> parseHexColor(std::string_view value) {
    if (value.size() == 7 && value.front() == '#') {
        value.remove_prefix(1);
    }
    if (value.size() != 6) {
        return std::nullopt;
    }

    Color result = 0;
    for (const char character : value) {
        result <<= 4U;
        if (character >= '0' && character <= '9') {
            result |= static_cast<Color>(character - '0');
        } else if (character >= 'a' && character <= 'f') {
            result |= static_cast<Color>(character - 'a' + 10);
        } else if (character >= 'A' && character <= 'F') {
            result |= static_cast<Color>(character - 'A' + 10);
        } else {
            return std::nullopt;
        }
    }
    return result;
}

std::string formatHexColor(Color value) {
    std::ostringstream output;
    output << '#' << std::uppercase << std::hex << std::setfill('0') << std::setw(6) << (value & 0xFFFFFFU);
    return output.str();
}

double contrastRatio(Color foreground, Color background) {
    const auto luminance = [](Color color) {
        const auto red = static_cast<std::uint8_t>((color >> 16U) & 0xFFU);
        const auto green = static_cast<std::uint8_t>((color >> 8U) & 0xFFU);
        const auto blue = static_cast<std::uint8_t>(color & 0xFFU);
        return 0.2126 * channelLuminance(red) + 0.7152 * channelLuminance(green) + 0.0722 * channelLuminance(blue);
    };

    const double first = luminance(foreground);
    const double second = luminance(background);
    const double lighter = std::max(first, second);
    const double darker = std::min(first, second);
    return (lighter + 0.05) / (darker + 0.05);
}

std::vector<std::string> validateProfile(const ThemeProfile& profile) {
    std::vector<std::string> errors;
    if (profile.schemaVersion != 1 && profile.schemaVersion != 2) {
        errors.emplace_back("schemaVersion must be 1 or 2");
    }
    if (profile.id.size() > 128 || !validIdentifier(profile.id)) {
        errors.emplace_back("id must contain 1-128 ASCII letters, digits, dots, underscores, or hyphens");
    }
    if (profile.name.empty() || profile.name.size() > 128 || hasControlCharacters(profile.name)) {
        errors.emplace_back("name must contain 1-128 characters");
    }
    if (profile.fontFamily.empty() || profile.fontFamily.size() > 128 || hasControlCharacters(profile.fontFamily)) {
        errors.emplace_back("fontFamily must contain 1-128 characters");
    }
    if (profile.fontSizePt < 6 || profile.fontSizePt > 48) {
        errors.emplace_back("fontSizePt must be between 6 and 48");
    }
    if (profile.schemaVersion == 1 && profile.shell) {
        errors.emplace_back("schemaVersion 1 cannot contain shell settings");
    }
    if (profile.schemaVersion == 2 && !profile.shell) {
        errors.emplace_back("schemaVersion 2 requires shell settings");
    }
    if (profile.shell) {
        if (profile.shell->fontFamily.empty() || profile.shell->fontFamily.size() > 128 ||
            hasControlCharacters(profile.shell->fontFamily)) {
            errors.emplace_back("shell fontFamily must contain 1-128 characters");
        }
        if (profile.shell->fontSizePt < 6 || profile.shell->fontSizePt > 48) {
            errors.emplace_back("shell fontSizePt must be between 6 and 48");
        }
        if (profile.shell->colorOverrides.size() > 45) {
            errors.emplace_back("shell colors cannot contain more than 45 roles");
        }
        for (const auto& [role, color] : profile.shell->colorOverrides) {
            static_cast<void>(color);
            if (!isShellColorRole(role)) {
                errors.emplace_back("unknown shell color role: " + role);
            }
        }
    }
    if (contrastRatio(profile.palette.foreground, profile.palette.background) < 3.0) {
        errors.emplace_back("foreground/background contrast must be at least 3.0:1");
    }
    if (contrastRatio(profile.palette.caret, profile.palette.background) < 3.0) {
        errors.emplace_back("caret/background contrast must be at least 3.0:1");
    }
    if (profile.schemaVersion == 2 && profile.shell) {
        for (const auto& shellError : validateShellPalette(deriveShellPalette(profile))) {
            errors.emplace_back("shell palette: " + shellError);
        }
    }
    return errors;
}

ThemeProfile migrateProfileToV2(const ThemeProfile& profile) {
    const auto errors = validateProfile(profile);
    if (!errors.empty()) {
        throw std::invalid_argument(errors.front());
    }
    ThemeProfile migrated = profile;
    migrated.schemaVersion = 2;
    if (!migrated.shell) {
        migrated.shell = ShellSettings{};
    }
    return migrated;
}

std::string serializeProfile(const ThemeProfile& profile) {
    json palette;
    paletteToJson(palette, profile.palette);
    json document = {
        {"schemaVersion", profile.schemaVersion},
        {"id", profile.id},
        {"name", profile.name},
        {"mode", profile.dark ? "dark" : "light"},
        {"palette", palette},
        {"typography", {{"fontFamily", profile.fontFamily}, {"fontSizePt", profile.fontSizePt}}},
    };
    if (profile.schemaVersion == 2 && profile.shell) {
        json colors = json::object();
        for (const auto& [role, color] : profile.shell->colorOverrides) {
            colors[role] = formatHexColor(color);
        }
        document["shell"] = {
            {"density", densityName(profile.shell->density)},
            {"typography", {{"fontFamily", profile.shell->fontFamily}, {"fontSizePt", profile.shell->fontSizePt}}},
            {"colors", std::move(colors)},
        };
    }
    return document.dump(2) + '\n';
}

ThemeProfile deserializeProfile(std::string_view jsonText) {
    if (jsonText.size() > 1024U * 1024U) {
        throw std::invalid_argument("Profile exceeds 1 MiB limit");
    }
    if (!nestingWithinLimit(jsonText, 64)) {
        throw std::invalid_argument("Profile exceeds 64-level nesting limit");
    }

    const json document = json::parse(jsonText.begin(), jsonText.end());
    ThemeProfile profile;
    profile.schemaVersion = document.at("schemaVersion").get<int>();
    profile.id = document.at("id").get<std::string>();
    profile.name = document.at("name").get<std::string>();
    const auto mode = document.at("mode").get<std::string>();
    if (mode != "dark" && mode != "light") {
        throw std::invalid_argument("mode must be dark or light");
    }
    profile.dark = mode == "dark";
    profile.palette = paletteFromJson(document.at("palette"));
    profile.fontFamily = document.at("typography").at("fontFamily").get<std::string>();
    profile.fontSizePt = document.at("typography").at("fontSizePt").get<int>();
    if (profile.schemaVersion == 2) {
        const auto& shell = document.at("shell");
        ShellSettings settings;
        settings.density = densityFromName(shell.at("density").get<std::string>());
        settings.fontFamily = shell.at("typography").at("fontFamily").get<std::string>();
        settings.fontSizePt = shell.at("typography").at("fontSizePt").get<int>();
        for (const auto& [role, encoded] : shell.at("colors").items()) {
            if (!isShellColorRole(role)) {
                throw std::invalid_argument("unknown shell color role: " + role);
            }
            const auto color = parseHexColor(encoded.get<std::string>());
            if (!color) {
                throw std::invalid_argument("invalid shell color role: " + role);
            }
            settings.colorOverrides.emplace(role, *color);
        }
        profile.shell = std::move(settings);
    }

    const auto errors = validateProfile(profile);
    if (!errors.empty()) {
        throw std::invalid_argument(errors.front());
    }
    return profile;
}

Color semanticColorForStyle(const ThemeProfile& profile, std::string_view language, int styleId,
                            Color existingForeground) {
    if (styleId == 0 || styleId == 32) {
        return profile.palette.foreground;
    }
    if (const auto role = roleForStyle(language, styleId)) {
        return colorForRole(profile.palette, *role);
    }
    return contrastRatio(existingForeground, profile.palette.background) >= 3.0 ? existingForeground
                                                                                : profile.palette.foreground;
}

} // namespace nppthemes
