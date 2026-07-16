#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>

#include "nppthemes/ShellPalette.h"
#include "nppthemes/ThemeProfile.h"

namespace {

constexpr std::uintmax_t maxProfileBytes = 1024U * 1024U;

[[nodiscard]] std::string readBoundedFile(const std::filesystem::path& path) {
    std::error_code error;
    const auto size = std::filesystem::file_size(path, error);
    if (error) {
        throw std::runtime_error("unable to inspect file");
    }
    if (size > maxProfileBytes) {
        throw std::invalid_argument("file exceeds 1 MiB limit");
    }
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("unable to open file");
    }
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

[[nodiscard]] std::string resolveProfile(const std::filesystem::path& path) {
    const auto profile = nppthemes::deserializeProfile(readBoundedFile(path));
    const auto palette = nppthemes::deriveShellPalette(profile);
    const auto errors = nppthemes::validateShellPalette(palette);
    if (!errors.empty()) {
        throw std::invalid_argument(errors.front());
    }
    return nppthemes::serializeShellPalette(palette);
}

[[nodiscard]] std::string normalizeLineEndings(std::string text) {
    std::string normalized;
    normalized.reserve(text.size());
    for (std::size_t index = 0; index < text.size(); ++index) {
        if (text[index] == '\r' && index + 1 < text.size() && text[index + 1] == '\n') {
            continue;
        }
        normalized.push_back(text[index]);
    }
    return normalized;
}

void printUsage() {
    std::cerr << "Usage: NppThemesConformance <profile.json>\n"
                 "       NppThemesConformance --check <profile.json> <expected.json>\n";
}

} // namespace

int main(int argc, char** argv) {
    const bool check = argc == 4 && std::string_view{argv[1]} == "--check";
    if (argc != 2 && !check) {
        printUsage();
        return 2;
    }

    try {
        const auto actual = resolveProfile(check ? argv[2] : argv[1]);
        if (!check) {
            std::cout << actual;
            return 0;
        }

        if (actual != normalizeLineEndings(readBoundedFile(argv[3]))) {
            std::cerr << "Conformance mismatch: canonical token output differs from golden fixture\n";
            return 1;
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Conformance failed: " << error.what() << '\n';
        return 1;
    }
}
