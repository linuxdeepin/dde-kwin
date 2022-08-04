set(TARGET_PATH "${CMAKE_INSTALL_PREFIX}/bin")
set(TARGET_NAME "deepin-display-screen")

find_package(Qt5Core REQUIRED)
find_package(Qt5DBus REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(X11 REQUIRED)
find_package(DtkCore REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(KF5Wayland REQUIRED)
find_package(Qt5X11Extras REQUIRED)

add_subdirectory(wayland)
add_subdirectory(x11)

install_files(
    "/share/dbus-1/interfaces"
    FILES
    com.deepin.display.screen.xml
)

qt5_add_dbus_adaptor(
    screen_SRCS
    com.deepin.display.screen.xml
    abstract_screen.h
    AbstractScreen
)

add_library(
    ${TARGET_NAME} SHARED
    abstract_screen.cpp
    wayland/wayland_screen.cpp
    x11/x11_screen.cpp
    ${screen_SRCS}
)

target_include_directories(
    ${TARGET_NAME}
    PRIVATE
    ${GSETTINGS_INCLUDE_DIRS}
)

target_link_libraries(
    ${TARGET_NAME}
    Qt5::Core
    Qt5::DBus
    Qt5::Gui
    Qt5::X11Extras
    dtkcore
    KF5::WaylandServer
    KF5::WaylandClient
)

install_targets("/lib" ${TARGET_NAME})
