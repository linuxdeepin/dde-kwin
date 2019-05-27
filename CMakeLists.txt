project(DDEKWin)

set(PROJECT_NAME "dde-kwin")
add_definitions(-DPROJECT_NAME=\\"${PROJECT_NAME}\\")

if (NOT PROJECT_VERSION)
    set(PROJECT_VERSION "0.0.1")
endif()

if (NOT PROJECT_VERSION_MAJOR)
    set(PROJECT_VERSION_MAJOR 0)
endif()

find_package(Qt5Core)
get_target_property(QT_QMAKE_EXECUTABLE ${Qt5Core_QMAKE_EXECUTABLE} IMPORTED_LOCATION)

macro(query_qmake args output)
    exec_program(${QT_QMAKE_EXECUTABLE} ARGS -query ${args} OUTPUT_VARIABLE ${output} RETURN_VALUE exitCode)

    if(NOT ${exitCode} EQUAL 0)
        message(FATAL_ERROR "exec ${QT_QMAKE_EXECUTABLE} failed, with args: ${args}, error message: ${output}")
    endif()
endmacro()

query_qmake("QT_INSTALL_PLUGINS" QT_INSTALL_PLUGINS)

set(PLUGIN_INSTALL_PATH ${QT_INSTALL_PLUGINS}/platforms)
# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed
set(CMAKE_AUTOMOC ON)
# Create code from a list of Qt designer ui files
set(CMAKE_AUTOUIC ON)

add_subdirectory(configures)
add_subdirectory(plugins)
add_subdirectory(aurorae-themes)
add_subdirectory(scripts)
add_subdirectory(deepin-wm-dbus)
add_subdirectory(tabbox)
