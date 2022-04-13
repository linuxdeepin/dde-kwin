

set(TARGET_PATH "${CMAKE_INSTALL_PREFIX}/bin")
set(TARGET_NAME "deepin-display")

find_package(Qt5Core REQUIRED)
find_package(Qt5DBus REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(X11 REQUIRED)
find_package(DtkCore REQUIRED)
find_package(PkgConfig REQUIRED)

configure_file(com.deepin.display.service.in com.deepin.display.service)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/mouse)
add_subdirectory(mouse)

install_files(
    "/share/dbus-1/services"
    FILES
    ${CMAKE_CURRENT_BINARY_DIR}/com.deepin.display.service
)

add_executable(
    ${TARGET_NAME}
    main.cpp
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
    deepin-display-mouse
)

install_targets("/bin" ${TARGET_NAME})
