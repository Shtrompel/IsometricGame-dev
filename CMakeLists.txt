cmake_minimum_required(VERSION 3.10)
project(SFML_TGUI_Game LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


set(CMAKE_BUILD_TYPE Debug)
add_compile_options(-fsanitize=address -g -fno-omit-frame-pointer)
add_link_options(-fsanitize=address)

# Find Boost
find_package(Boost REQUIRED COMPONENTS program_options)

# Find SFML and TGUI
find_package(SFML COMPONENTS Graphics Window System REQUIRED)
find_package(TGUI REQUIRED)

# Collect all source files from the src directory
file(GLOB_RECURSE SOURCES
    "src/*.cpp"
    "src/*.c"
)

# Note: GameWindow.cpp is located in the include directory based on your tree
list(APPEND SOURCES "include/window/game_window.hpp")

# Define the executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Specify include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/libs
)

# Link the required libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    Boost::program_options
    SFML::Graphics
    SFML::Window
    SFML::System
    TGUI::TGUI
)
