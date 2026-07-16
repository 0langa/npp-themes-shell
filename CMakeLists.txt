add_library(NppThemesCore STATIC
    src/ShellPalette.cpp
    src/ThemeProfile.cpp
    src/ThemeXmlCompiler.cpp
)

target_include_directories(NppThemesCore PUBLIC include)
target_link_libraries(NppThemesCore PUBLIC nlohmann_json::nlohmann_json pugixml::pugixml)
target_compile_features(NppThemesCore PUBLIC cxx_std_20)

if(MSVC)
    target_compile_options(NppThemesCore PRIVATE /W4 /WX /permissive- /utf-8 /EHsc)
endif()
