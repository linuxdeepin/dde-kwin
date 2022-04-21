set(TARGET_PATH "${CMAKE_INSTALL_PREFIX}/bin")
set(TARGET_NAME "deepin-display-output")

find_package(Qt5Core REQUIRED)
find_package(Qt5DBus REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(X11 REQUIRED)
find_package(DtkCore REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(KF5Wayland REQUIRED)

add_subdirectory(wayland)
add_subdirectory(x11)

install_files(
    "/share/dbus-1/interfaces"
    FILES
    com.deepin.display.output.xml
)

qt5_add_dbus_adaptor(
    output_SRCS
    com.deepin.display.output.xml
    abstract_output.h
    AbstractOutput
)

add_library(
    ${TARGET_NAME} SHARED
    abstract_output.cpp
    wayland/wayland_output.cpp
    x11/x11_output.cpp
    ${output_SRCS}
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
    KF5::WaylandServer
    KF5::WaylandClient
)

install_targets("/lib" ${TARGET_NAME})

