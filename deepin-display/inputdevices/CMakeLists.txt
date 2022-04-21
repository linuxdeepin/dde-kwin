set(TARGET_PATH "${CMAKE_INSTALL_PREFIX}/bin")
set(TARGET_NAME "deepin-display-inputdevices")

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
    com.deepin.display.inputdevices.xml
)

qt5_add_dbus_adaptor(
    inputdevices_SRCS
    com.deepin.display.inputdevices.xml
    abstract_inputdevices.h
    AbstractInputDevices
)

add_library(
    ${TARGET_NAME} SHARED
    abstract_inputdevices.cpp
    wayland/wayland_inputdevices.cpp
    x11/x11_inputdevices.cpp
    ${inputdevices_SRCS}
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
