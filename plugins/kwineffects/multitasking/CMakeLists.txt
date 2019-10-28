set(PLUGIN_NAME "multitasking")

find_package(Qt5Quick REQUIRED)
find_package(Qt5QuickWidgets REQUIRED)
find_package(KF5Config REQUIRED)
find_package(KF5GlobalAccel REQUIRED)
find_package(KF5I18n REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_search_module(QGSETTINGS REQUIRED IMPORTED_TARGET gsettings-qt)

add_library(
    ${PLUGIN_NAME} MODULE
    multitasking.cpp
    background.cpp
    factory.cpp
    data.qrc
)

target_link_libraries(
    ${PLUGIN_NAME}
    PUBLIC
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Quick
    Qt5::QuickWidgets
    PRIVATE
    KF5::ConfigCore
    KF5::CoreAddons
    KF5::WindowSystem
    KF5::GlobalAccel
    KF5::I18n
    PkgConfig::QGSETTINGS
)

install(TARGETS ${PLUGIN_NAME} DESTINATION ${INSTALL_PATH})
