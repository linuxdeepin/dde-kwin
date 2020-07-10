set(TARGET_NAME "blur")

find_package(KF5Config REQUIRED)

add_library(
    ${TARGET_NAME} MODULE
    blur.cpp
    blurshader.cpp
    main.cpp
)

include_directories(../../platforms/lib)

target_link_libraries(
    ${TARGET_NAME}
    PUBLIC
    Qt5::Core
    Qt5::Gui
    PRIVATE
    KF5::CoreAddons
    KF5::WindowSystem
    KF5::ConfigCore
)

install(TARGETS ${TARGET_NAME} DESTINATION ${INSTALL_PATH})
