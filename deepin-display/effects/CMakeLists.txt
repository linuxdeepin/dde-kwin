set(TARGET_PATH "${CMAKE_INSTALL_PREFIX}/bin")
set(TARGET_NAME "deepin-display-effects")

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
    com.deepin.display.effects.xml
)

qt5_add_dbus_adaptor(
    effects_SRCS
    com.deepin.display.effects.xml
    abstract_effects.h
    AbstractEffects
)

add_library(
    ${TARGET_NAME} SHARED
    abstract_effects.cpp
    wayland/wayland_effects.cpp
    x11/x11_effects.cpp
    ${effects_SRCS}
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
