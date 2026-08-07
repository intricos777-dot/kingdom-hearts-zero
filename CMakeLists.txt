cmake_minimum_required(VERSION 3.20)
project(kingdom-hearts-zero VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(nlohmann_json CONFIG REQUIRED)
find_package(SDL2 REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2_ttf REQUIRED IMPORTED_TARGET SDL2_ttf)
pkg_check_modules(GLEW REQUIRED IMPORTED_TARGET glew)
find_package(OpenGL REQUIRED)

# Built entirely off the Twilight Elysium engine
add_subdirectory(/home/sin/Projects/twilight-elysium ${CMAKE_BINARY_DIR}/twilight-elysium)

# World-synced command deck UI (terminal)
add_library(khz-ui STATIC
    src/ui/command_deck.cpp
)
target_include_directories(khz-ui PUBLIC src)
target_link_libraries(khz-ui PRIVATE te-engine)

# Story / Ansem-narrated prologue + scene graph
add_library(khz-story STATIC
    src/story/scene.cpp
)
target_include_directories(khz-story PUBLIC src)
target_link_libraries(khz-story PRIVATE te-engine khz-ui)

# Save system with checksums
add_library(khz-save STATIC
    src/save/save_system.cpp
)
target_include_directories(khz-save PUBLIC src)
target_link_libraries(khz-save PRIVATE te-engine)

# World database (KH1 + KH2 manifests)
add_library(khz-worlds STATIC
    src/render/worlds.cpp
)
target_include_directories(khz-worlds PUBLIC src)
target_link_libraries(khz-worlds PRIVATE te-engine nlohmann_json::nlohmann_json)

# Visual shell: SDL2/GL renderer + Tron selection hub
add_library(khz-render STATIC
    src/render/renderer.cpp
    src/ui/tron_shell.cpp
)
target_include_directories(khz-render PUBLIC src)
target_link_libraries(khz-render PRIVATE te-engine khz-worlds
    SDL2::SDL2 PkgConfig::SDL2_ttf PkgConfig::GLEW OpenGL::GL)

# Main executable
enable_testing()
add_executable(kh-door src/main.cpp)
target_include_directories(kh-door PRIVATE src)
target_link_libraries(kh-door PRIVATE te-engine khz-ui khz-story khz-save khz-worlds khz-render nlohmann_json::nlohmann_json)
add_test(NAME khz-core COMMAND kh-door --terminal)
