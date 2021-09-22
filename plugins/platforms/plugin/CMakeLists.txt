set(TARGET_NAME "${PROJECT_NAME}-xcb")
set(WAYLAND_TARGET_NAME "${PROJECT_NAME}-wayland")

# 禁止语法树上的 vrp 优化，-O2/-O3 默认开启，会导致测试虚析构函数 HOOK 失败
add_compile_options(-fno-tree-vrp)

find_package(Qt5Core REQUIRED)
find_package(Qt5Quick REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(Qt5DBus REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(DtkGui REQUIRED)
find_package(X11 REQUIRED)

include_directories(${Qt5Core_PRIVATE_INCLUDE_DIRS})
include_directories(${Qt5Gui_PRIVATE_INCLUDE_DIRS})
include_directories(../lib)


option(USE_DBUS_MENU "use deepin dbus menu for the window title menu widget" OFF)
option(DISABLE_DXCB "disable DXcb platform plugin" OFF)

if (USE_DBUS_MENU)
    add_definitions(-DUSE_DBUS_MENU)
endif()

if (DISABLE_DXCB)
    add_definitions(-DDISABLE_DXCB)
endif()

qt5_add_dbus_adaptor(
    plugin_SRCS
    org.kde.kwin.KWinUtils.xml
    kwinutils.h
    KWinUtils
)

install_files(
    "/share/dbus-1/interfaces"
    FILES
    org.kde.kwin.KWinUtils.xml
)

# for xcb platform
add_library(
    ${TARGET_NAME} MODULE
    main.cpp
    libkwinpreload.cpp
    ${plugin_SRCS}
)

target_compile_definitions(
    ${TARGET_NAME}
    PRIVATE
    PLATFORMS_PLUGIN_PATH="${PLUGIN_INSTALL_PATH}"
    TARGET_NAME="${TARGET_NAME}"
)

target_link_libraries(
    ${TARGET_NAME}
    Qt5::Core
    Qt5::DBus
    Qt5::Quick
    Qt5::Widgets
    ${DtkGui_LIBRARIES}
    kwin-xcb
)

# for wayland platform
add_library(
    ${WAYLAND_TARGET_NAME} MODULE
    main_wayland.cpp
    libkwinpreload.cpp
    ${plugin_SRCS}
)

target_compile_definitions(
    ${WAYLAND_TARGET_NAME}
    PRIVATE
    PLATFORMS_PLUGIN_PATH="${PLUGIN_INSTALL_PATH}"
    TARGET_NAME="${WAYLAND_TARGET_NAME}"
    TRANSLATE_NAME="${TARGET_NAME}"
    WAYLAND_PLATFORM=1
)

target_link_libraries(
    ${WAYLAND_TARGET_NAME}
    Qt5::Core
    Qt5::DBus
    Qt5::Quick
    Qt5::Widgets
    kwin-xcb
)
install(TARGETS ${TARGET_NAME} ${WAYLAND_TARGET_NAME} DESTINATION ${PLUGIN_INSTALL_PATH})

if (NOT CMAKE_BUILD_TYPE MATCHES Debug)
    exec_program(${CMAKE_CURRENT_SOURCE_DIR}/translate_generation.sh OUTPUT_VARIABLE output RETURN_VALUE exit_code)

    if (NOT exit_code EQUAL 0)
        message(SEND_ERROR "Failed to generate translation, exit code=${exit_code}, message=\"${output}\"")
    endif()

    file(GLOB QM_FILES "translations/*.qm")
    install_files("/share/${TARGET_NAME}/translations" FILES ${QM_FILES})
endif()
