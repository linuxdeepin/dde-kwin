project(DDEKWin)

set(PROJECT_NAME "dde-kwin")

if (NOT PROJECT_VERSION)
    set(PROJECT_VERSION "0.0.1")
endif()

if (NOT PROJECT_VERSION_MAJOR)
    set(PROJECT_VERSION_MAJOR 0)
endif()

add_definitions(
    -DPROJECT_NAME=\\"${PROJECT_NAME}\\"
    -DPROJECT_VERSION=\\"${PROJECT_VERSION}\\"
)

option(USE_SCRIPTS "use scripts" ON)
option(USE_DEEPIN_WM_DBUS "use deepin-wm dbus service" ON)
option(USE_DEEPIN_DISPLAY "use deepin-display dbus service" ON)
option(BUILD_TESTING "default build testing" ON)

#when "KWIN_VERSION > KWIN_VERSION_CHECK(5, 18, 90, 0)"
find_path(
    KWaylandServerPath
    blur_interface.h
    /usr/include/KWaylandServer
    /usr/local/include/KWaylandServer
    )
if(KWaylandServerPath)
    add_definitions(-DKWaylandServerPath)
endif()

if (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "mips64")
    set(CMAKE_CXX_FLAGS "-O3 -ftree-vectorize -march=loongson3a -mhard-float -mno-micromips -mno-mips16 -flax-vector-conversions  -mloongson-ext2 -mloongson-mmi -Wl,--as-needed,-O1 -fno-stack-protector")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--as-need -fPIE")
    # 1050 security testing,security hardening
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fstack-protector-all -z relro -z now -z noexecstack -pie")
    set(CMAKE_EXE_LINKER_FLAGS "-pie")
endif()

find_package(Qt5Core)
get_target_property(QT_QMAKE_EXECUTABLE ${Qt5Core_QMAKE_EXECUTABLE} IMPORTED_LOCATION)

macro(query_qmake args output)
    exec_program(${QT_QMAKE_EXECUTABLE} ARGS -query ${args} OUTPUT_VARIABLE ${output} RETURN_VALUE exitCode)

    if(NOT ${exitCode} EQUAL 0)
        message(FATAL_ERROR "exec ${QT_QMAKE_EXECUTABLE} failed, with args: ${args}, error message: ${output}")
    endif()
endmacro()

include(GNUInstallDirs)
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX /usr)
endif ()

if(NOT DEFINED QT_INSTALL_PLUGINS)
    query_qmake("QT_INSTALL_PLUGINS" QT_INSTALL_PLUGINS)
endif()

set(PLUGIN_INSTALL_PATH ${QT_INSTALL_PLUGINS}/platforms)
# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed
set(CMAKE_AUTOMOC ON)
# Create code from a list of Qt designer ui files
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

if (NOT KWIN_VERSION)
    find_package(KDecoration2 REQUIRED)
    set(KWIN_VERSION ${KDecoration2_VERSION})
endif()

if (KWIN_VERSION)
    message("kwin version: ${KWIN_VERSION}")
    string(REPLACE "." ";" KWIN_VERSION_LIST ${KWIN_VERSION})
    list(APPEND KWIN_VERSION_LIST 0 0 0 0)

    if (NOT KWIN_VER_MAJ)
        list(GET KWIN_VERSION_LIST 0 KWIN_VER_MAJ)
    endif()

    if (NOT KWIN_VER_MIN)
        list(GET KWIN_VERSION_LIST 1 KWIN_VER_MIN)
    endif()

    if (NOT KWIN_VER_PAT)
        list(GET KWIN_VERSION_LIST 2 KWIN_VER_PAT)
    endif()

    if (NOT KWIN_VER_BUI)
        list(GET KWIN_VERSION_LIST 3 KWIN_VER_BUI)
    endif()

    add_definitions(
        -DKWIN_VERSION_STR=\\"${KWIN_VERSION}\\"
        -DKWIN_VERSION_MAJ=${KWIN_VER_MAJ}
        -DKWIN_VERSION_MIN=${KWIN_VER_MIN}
        -DKWIN_VERSION_PAT=${KWIN_VER_PAT}
        -DKWIN_VERSION_BUI=${KWIN_VER_BUI}
    )
endif()

add_subdirectory(configures)

if (USE_SCRIPTS)
  add_subdirectory(scripts)
endif()

if (USE_DEEPIN_WM_DBUS)
  add_subdirectory(deepin-wm-dbus)
endif()

if (USE_DEEPIN_DISPLAY)
  add_subdirectory(deepin-display)
endif()

if (BUILD_TESTING)
    add_subdirectory(tests)
endif()
