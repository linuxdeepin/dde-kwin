set(TARGET_NAME "deepin-chameleon")

add_definitions(-DTARGET_NAME=\\"${TARGET_NAME}\\")

find_package(Qt5 CONFIG REQUIRED COMPONENTS Core)
find_package(KF5CoreAddons REQUIRED)
find_package(KF5Config REQUIRED)
find_package(KDecoration2 REQUIRED)

option(USE_LIB_KWIN_XCB "Use libkwin-xcb" OFF)

set(CMAKE_CXX_STANDARD 11)

add_library(
    ${TARGET_NAME}
    MODULE
    main.cpp
    chameleon.cpp
    chameleontheme.cpp
    chameleonbutton.cpp
    chameleonshadow.cpp
    themes.qrc
)

target_link_libraries(
    ${TARGET_NAME}
    PUBLIC
    Qt5::Core
    PRIVATE
    KDecoration2::KDecoration
    KF5::CoreAddons
    KF5::ConfigCore
)

if (USE_LIB_KWIN_XCB)
    include_directories(../kwin-xcb/lib)
    target_link_libraries(${TARGET_NAME} kwin-xcb)
else()
    add_definitions(-DDISBLE_DDE_KWIN_XCB)
endif()

install(TARGETS ${TARGET_NAME} DESTINATION ${QT_INSTALL_PLUGINS}/org.kde.kdecoration2)
