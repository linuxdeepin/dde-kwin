set(TARGET_PATH "${CMAKE_INSTALL_PREFIX}/bin")
set(TARGET_NAME "deepin-display-mouse")

find_package(Qt5Core REQUIRED)
find_package(Qt5DBus REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(X11 REQUIRED)
find_package(DtkCore REQUIRED)
find_package(PkgConfig REQUIRED)

add_subdirectory(wayland)
add_subdirectory(x11)

install_files(
    "/share/dbus-1/interfaces"
    FILES
    com.deepin.display.mouse.xml
)

qt5_add_dbus_adaptor(
    mouse_SRCS
    com.deepin.display.mouse.xml
    abstract_mouse.h
    AbstractMouse
)

add_library(
    ${TARGET_NAME} SHARED
    abstract_mouse.cpp
    wayland/wayland_mouse.cpp
    x11/x11_mouse.cpp
    ${mouse_SRCS}
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
    dtkcore
)

install_targets("/lib" ${TARGET_NAME})
