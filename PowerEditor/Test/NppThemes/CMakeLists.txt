cmake_minimum_required(VERSION 3.24)

project(NppThemesShellTests LANGUAGES CXX)

include(CTest)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(NPP_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../src")
set(NPP_THEMES_CORE_DIR "${NPP_SOURCE_DIR}/NppThemesCore")

add_library(NppThemesShellCore STATIC
    "${NPP_THEMES_CORE_DIR}/src/ShellPalette.cpp"
    "${NPP_THEMES_CORE_DIR}/src/ThemeProfile.cpp"
)
target_include_directories(NppThemesShellCore PUBLIC
    "${NPP_THEMES_CORE_DIR}/include"
    "${NPP_SOURCE_DIR}"
)

add_executable(NppThemesConformance
    "${NPP_THEMES_CORE_DIR}/tools/shell-conformance.cpp"
)
target_link_libraries(NppThemesConformance PRIVATE NppThemesShellCore)

add_executable(NppThemesThemeServiceTests
    ThemeServiceTests.cpp
    "${NPP_SOURCE_DIR}/NppThemes/DarkModeThemeAdapter.cpp"
    "${NPP_SOURCE_DIR}/NppThemes/ThemeService.cpp"
)
target_include_directories(NppThemesThemeServiceTests PRIVATE "${NPP_SOURCE_DIR}")
target_link_libraries(NppThemesThemeServiceTests PRIVATE NppThemesShellCore)

if(MSVC)
    foreach(target NppThemesShellCore NppThemesConformance NppThemesThemeServiceTests)
        target_compile_options(${target} PRIVATE /W4 /WX /permissive- /utf-8 /EHsc)
    endforeach()
endif()

add_test(NAME shell-conformance COMMAND NppThemesConformance --check
    "${NPP_THEMES_CORE_DIR}/tests/fixtures/profile-v2.json"
    "${NPP_THEMES_CORE_DIR}/tests/fixtures/profile-v2.expected.json"
)
add_test(NAME theme-service COMMAND NppThemesThemeServiceTests)
