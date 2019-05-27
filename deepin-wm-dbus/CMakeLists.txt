set(TARGET_PATH "${CMAKE_INSTALL_PREFIX}/bin")
set(TARGET_NAME "deepin-wm-dbus")

find_package(Qt5Core REQUIRED)
find_package(Qt5DBus REQUIRED)
find_package(KF5Config REQUIRED)
find_package(KF5WindowSystem REQUIRED)
find_package(KF5GlobalAccel REQUIRED)
find_package(DtkCore REQUIRED)
find_package(PkgConfig REQUIRED)

option(DISABLE_DEEPIN_WM "disable compatibility with deepin-wm" OFF)

if (DISABLE_DEEPIN_WM)
    set(DEEPIN_WM_PREFIX "<!-- By \"CONFIG+=DISABLE_DEEPIN_WM\"")
    set(DEEPIN_WM_SUFFIX "-->")
    add_definitions(-DDISABLE_DEEPIN_WM)
else()
    pkg_search_module(GSETTINGS REQUIRED gsettings-qt)
endif()

configure_file(com.deepin.wm.xml.in com.deepin.wm.xml)
configure_file(com.deepin.wm.service.in com.deepin.wm.service)

install_files(
    "/share/dbus-1/interfaces"
    FILES
    ${CMAKE_CURRENT_BINARY_DIR}/com.deepin.wm.xml
)
install_files(
    "/share/dbus-1/services"
    FILES
    ${CMAKE_CURRENT_BINARY_DIR}/com.deepin.wm.service
)

qt5_add_dbus_adaptor(
    wm_SRCS
    ${CMAKE_CURRENT_BINARY_DIR}/com.deepin.wm.xml
    deepinwmfaker.h
    DeepinWMFaker
)
qt5_add_dbus_interface(
    wm_SRCS
    ../plugins/kwin-xcb/plugin/org.kde.kwin.KWinUtils.xml
    kwinutils_interface
)

add_executable(
    ${TARGET_NAME}
    deepinwmfaker.cpp
    main.cpp
    ${wm_SRCS}
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
    KF5::ConfigCore
    KF5::WindowSystem
    KF5::GlobalAccel
    dtkcore
    ${GSETTINGS_LDFLAGS}
)

install_targets("/bin" deepin-wm-dbus)
