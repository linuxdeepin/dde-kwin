project(DDEKWin)

set(PROJECT_NAME "dde-kwin")

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

if(NOT DEFINED PLUGIN_INSTALL_PATH)
    set(PLUGIN_INSTALL_PATH ${QT_INSTALL_PLUGINS}/platforms)
endif()

add_definitions(
    -DPROJECT_NAME=\\"${PROJECT_NAME}\\"
    -DPROJECT_VERSION=\\"${PROJECT_VERSION}\\"
)

option(USE_PLUGINS "use dde-kwin plugins" ON)
option(USE_SCRIPTS "use scripts" ON)
option(USE_DEEPIN_WM_DBUS "use deepin-wm dbus service" ON)
option(USE_TABBOX "use deepin style tabbox" ON)
option(ENABLE_BUILTIN_SCISSOR_WINDOW "enable deepin built-in scissor window plugin" ON)

# test deepin black screen
try_compile(TEST_DEEPIN_BLACK_SCREEN
              ${CMAKE_CURRENT_BINARY_DIR}/try_compile_black_screen
              ${CMAKE_CURRENT_SOURCE_DIR}/plugins/kwineffects/black-screen
              "multitasking"
              CMAKE_FLAGS
                "-DCMAKE_BUILD_TYPE=Debug"
)

message(STATUS "TEST_DEEPIN_BLACK_SCREEN ${TEST_DEEPIN_BLACK_SCREEN}")
if (TEST_DEEPIN_BLACK_SCREEN)
  option(ENABLE_BUILTIN_BLACK_SCREEN "enable deepin built-in black screen plugin" ON)
endif(TEST_DEEPIN_BLACK_SCREEN)

# test deepin multitasking
try_compile(TEST_DEEPIN_MULTITASKING
              ${CMAKE_CURRENT_BINARY_DIR}/try_compile_multitasking
              ${CMAKE_CURRENT_SOURCE_DIR}/plugins/kwineffects/multitasking
              "multitasking"
              CMAKE_FLAGS
                "-DCMAKE_BUILD_TYPE=Debug"
)

message(STATUS "TEST_DEEPIN_MULTITASKING ${TEST_DEEPIN_MULTITASKING}")
if (TEST_DEEPIN_MULTITASKING)
  option(ENABLE_BUILTIN_MULTITASKING "enable deepin built-in multitasking plugin" ON)
endif(TEST_DEEPIN_MULTITASKING)

# test deepin window tool
try_compile(TEST_DEEPIN_WINDOW_TOOL
              ${CMAKE_CURRENT_BINARY_DIR}/try_compile_window_tool
              ${CMAKE_CURRENT_SOURCE_DIR}/windowtool
              "dde-windowtool"
              CMAKE_FLAGS
                "-DCMAKE_BUILD_TYPE=Debug"
)

message(STATUS "TEST_DEEPIN_WINDOW_TOOL ${TEST_DEEPIN_WINDOW_TOOL}")
if (TEST_DEEPIN_WINDOW_TOOL)
  option(USE_WINDOW_TOOL "support wayland and x11" ON)
endif(TEST_DEEPIN_WINDOW_TOOL)

# test deepin blur
find_package(KF5WindowSystem REQUIRED)
find_package(Qt5 CONFIG REQUIRED COMPONENTS Core)
find_package(X11 REQUIRED)
try_compile(TEST_DEEPIN_BLUR
              ${CMAKE_CURRENT_BINARY_DIR}/try_compile_blur
            SOURCES
              ${CMAKE_CURRENT_SOURCE_DIR}/tests/blur_test.cc
            LINK_LIBRARIES
              Qt5::Core
              KF5::WindowSystem
              X11::X11
)

message(STATUS "TEST_DEEPIN_BLUR ${TEST_DEEPIN_BLUR}")
if (TEST_DEEPIN_BLUR)
  option(ENABLE_BUILTIN_BLUR "enable deepin built-in blur plugin" ON)
  if(ENABLE_BUILTIN_BLUR)
       add_definitions(-DENABLE_BUILTIN_BLUR)
  endif(ENABLE_BUILTIN_BLUR)
endif(TEST_DEEPIN_BLUR)

# test deepin wayland
find_package(KF5WindowSystem REQUIRED)
find_package(Qt5 CONFIG REQUIRED COMPONENTS Core)
find_package(X11 REQUIRED)
try_compile(TEST_DEEPIN_WAYLAND
              ${CMAKE_CURRENT_BINARY_DIR}/try_compile_wayland
            SOURCES
              ${CMAKE_CURRENT_SOURCE_DIR}/tests/wayland_test.cc
            LINK_LIBRARIES
              Qt5::Core
              KF5::WindowSystem
              X11::X11
)

message(STATUS "TEST_DEEPIN_WAYLAND ${TEST_DEEPIN_WAYLAND}")

if (TEST_DEEPIN_WAYLAND)
  option(USE_DEEPIN_WAYLAND "use deepin wayland" ON)
  if (USE_DEEPIN_WAYLAND)
      add_definitions(-DUSE_DEEPIN_WAYLAND)
  endif(USE_DEEPIN_WAYLAND)
endif(TEST_DEEPIN_WAYLAND)

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

if (USE_PLUGINS)
  add_subdirectory(plugins)
endif()

if (USE_SCRIPTS)
  add_subdirectory(scripts)
endif()

if (USE_DEEPIN_WM_DBUS)
  add_subdirectory(deepin-wm-dbus)
endif()

if (USE_TABBOX)
add_subdirectory(tabbox)
endif()

if (BUILD_TESTING)
    add_subdirectory(tests)
endif()

if (USE_WINDOW_TOOL)
  add_subdirectory(windowtool)
endif()

add_subdirectory(configures)
