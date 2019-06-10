set(TARGET_NAME "scissor-window")

add_library(
    ${TARGET_NAME} MODULE
    main.cpp
    scissorwindow.cpp
    glsl.qrc
)

target_link_libraries(
    ${TARGET_NAME}
    PUBLIC
    Qt5::Core
    Qt5::Gui
    PRIVATE
    KF5::CoreAddons
    KF5::WindowSystem
)

option(DISABLE_LIB_KWIN_XCB "Don't use libkwin-xcb" OFF)

if (DISABLE_LIB_KWIN_XCB)
    add_definitions(-DDISBLE_DDE_KWIN_XCB)
else()
    include_directories(../../kwin-xcb/lib)
    target_link_libraries(${TARGET_NAME} kwin-xcb)
endif()

install(TARGETS ${TARGET_NAME} DESTINATION ${INSTALL_PATH})
