set(TARGET_NAME "scissor-window")
find_package(X11 REQUIRED)

add_library(
    ${TARGET_NAME} MODULE
    main.cpp
    scissorwindow.cpp
    glsl.qrc
)

target_link_libraries(
    ${TARGET_NAME}
    PRIVATE
    KF5::CoreAddons
    KF5::WindowSystem
    PUBLIC
    Qt5::Core
    Qt5::Gui
    X11::X11
)

option(DISABLE_LIB_KWIN_XCB "Don't use libkwin-xcb" OFF)

if (DISABLE_LIB_KWIN_XCB)
    add_definitions(-DDISBLE_DDE_KWIN_XCB)
else()
    include_directories(../../platforms/lib)
    target_link_libraries(${TARGET_NAME} kwin-xcb)
endif()

install(TARGETS ${TARGET_NAME} DESTINATION ${INSTALL_PATH})
