cmake_minimum_required(VERSION 3.5)

set(PLUGIN_NAME "multitasking")
set(PLASMA_RELATIVE_DATA_INSTALL_DIR "plasma")

set(KF5_DEP_VERSION "5.54.0") # handled by release scripts

find_package(ECM 5.54.0  NO_MODULE)

set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})

find_package(KF5 ${KF5_DEP_VERSION} REQUIRED
    COMPONENTS
    OPTIONAL_COMPONENTS
        Wayland
)
set_package_properties(KF5Wayland PROPERTIES DESCRIPTION "Integration with the Wayland compositor"
                       TYPE OPTIONAL
                      )
if(KF5Wayland_FOUND)
    set(HAVE_KWAYLAND 1)
endif()

#optional features
find_package(X11 MODULE)
set_package_properties(X11 PROPERTIES DESCRIPTION "X11 libraries"
                       URL "http://www.x.org"
                       TYPE OPTIONAL
                      )
find_package(XCB MODULE COMPONENTS XCB COMPOSITE DAMAGE SHAPE XFIXES RENDER)
set_package_properties(XCB PROPERTIES DESCRIPTION "X protocol C-language Binding"
                       URL "http://xcb.freedesktop.org"
                       TYPE OPTIONAL
                      )
if(X11_FOUND AND XCB_XCB_FOUND)
  set(HAVE_X11 1)
  find_package(Qt5 REQUIRED NO_MODULE COMPONENTS X11Extras)
  #X11_Xrender discovery is done by FindX11
  #add_feature_info("X Rendering Extension (libXrender)" X11_Xrender_FOUND "Support for compositing, rendering operations, and alpha-blending. STRONGLY RECOMMENDED")
endif()

find_package(OpenGL)
set_package_properties(OpenGL PROPERTIES DESCRIPTION "The OpenGL libraries"
                       URL "http://www.opengl.org"
                       TYPE OPTIONAL
                      )

find_package(EGL)
set_package_properties(EGL PROPERTIES
                       PURPOSE "Support for Window Thumbnail on EGL platform"
                       TYPE OPTIONAL
                      )
set(HAVE_EGL ${EGL_FOUND})

if(OPENGL_FOUND AND (${Qt5Gui_OPENGL_IMPLEMENTATION} STREQUAL "GL"))
    set(HAVE_GLX ${HAVE_X11})
else()
    set(HAVE_GLX 0)
endif()

if(HAVE_X11 AND XCB_XCB_FOUND AND XCB_COMPOSITE_FOUND AND XCB_DAMAGE_FOUND)
    set(HAVE_XCB_COMPOSITE TRUE)
else()
    set(HAVE_XCB_COMPOSITE FALSE)
endif()

configure_file(config-x11.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-x11.h)
configure_file(config-plasma.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-plasma.h)

find_package(Qt5Quick REQUIRED)
find_package(Qt5QuickWidgets REQUIRED)
find_package(KF5Config REQUIRED)
find_package(KF5GlobalAccel REQUIRED)
find_package(KF5I18n REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(Qt5LinguistTools REQUIRED)

pkg_search_module(QGSETTINGS REQUIRED IMPORTED_TARGET gsettings-qt)

set(SRCS backgroundimageprovider.cpp imageprovider.cpp multitasking.cpp multitasking_model.cpp background.cpp
    factory.cpp windowthumbnail.cpp thumbmanager.qml PlusButton.qml DesktopBackgroundImage.qml)

# translations
file(GLOB TS LIST_DIRECTORIES false translations/${PLUGIN_NAME}*.ts)
set_source_files_properties(${TS}
    PROPERTIES OUTPUT_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/translations)

qt5_create_translation(QM ${SRCS} ${TS} OPTIONS -locations none)

add_library(
    ${PLUGIN_NAME} MODULE
    ${SRCS}
    ${QM}
    data.qrc
)

include_directories(../../platforms/lib)

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
    Qt5::X11Extras
)

install(TARGETS ${PLUGIN_NAME} DESTINATION ${INSTALL_PATH})
