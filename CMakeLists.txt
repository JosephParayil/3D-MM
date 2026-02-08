cmake_minimum_required(VERSION 3.16)
project(mm VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


set(TGUI_DIR "C:/Users/josep/Documents/Cpp/_PACKAGES/TGUI-1.12/build")
set(TGUI_STATIC_LIBRARIES TRUE)

include_directories(C:/Users/josep/Documents/Cpp/_PACKAGES/myLibs)

find_package(SFML 3 REQUIRED COMPONENTS System Window Graphics Audio)
find_package(TGUI 1 REQUIRED)

add_executable(mm main.cpp)

target_link_libraries(mm PRIVATE
    SFML::System
    SFML::Window
    SFML::Graphics
    SFML::Audio

    TGUI::TGUI
)

