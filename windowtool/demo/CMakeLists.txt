cmake_minimum_required(VERSION 3.0)

set(PROJECT_NAME "windowtool-Demo")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11 -Wall")

#Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

project(${PROJECT_NAME})

include_directories(SYSTEM ${Qt5Core_PRIVATE_INCLUDE_DIRS})

# Find the library
find_package(PkgConfig REQUIRED)
find_package(Qt5 COMPONENTS
    Core
    Gui
    Widgets
REQUIRED)

FILE(GLOB SC_FILES "*.cpp" "*.h")

add_executable(${PROJECT_NAME} ${SC_FILES})

target_link_libraries(${PROJECT_NAME}  Qt5::Core Qt5::Gui Qt5::Widgets)
target_link_libraries(${PROJECT_NAME}  dde-windowtool)
