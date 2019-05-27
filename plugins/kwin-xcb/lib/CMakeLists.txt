set(TARGET_NAME "kwin-xcb")

find_package(Qt5Core REQUIRED)
find_package(Qt5X11Extras REQUIRED)
find_package(KF5Config REQUIRED)
find_package(KF5WindowSystem REQUIRED)
find_package(KF5CoreAddons REQUIRED)

if (${KF5WindowSystem_VERSION_MINOR} GREATER 45)
    add_definitions(-DKWINDOWSYSTEM_NO_QWIDGET)
else()
    find_package(Qt5Widgets REQUIRED)
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
        -DKWIN_VERSION_PAT=${KWIN_VER_MIN}
        -DKWIN_VERSION_BUI=${KWIN_VER_BUI}
    )
endif()

add_library(
    ${TARGET_NAME} SHARED
    kwinutils.cpp
    vtablehook.cpp
)

set_target_properties(
    ${TARGET_NAME} PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

target_link_libraries(
    ${TARGET_NAME}
    Qt5::Core
    Qt5::X11Extras
    KF5::ConfigCore
    KF5::WindowSystem
    KF5::CoreAddons
)

if (KWIN_LIBRARY_PATH)
    # 强制链接libkwin.so，kwin开发库未提供 libkwin.so，需要构建脚本自行创建链接到 /usr/lib/.../libkwin.so.x.x.x 的软连接
    find_library(lib_kwin NAMES kwin PATHS ${KWIN_LIBRARY_PATH} NO_DEFAULT_PATH)
    if (EXISTS ${lib_kwin})
        target_link_libraries(${TARGET_NAME} ${lib_kwin})
    else()
        message(SEND_ERROR "Can't find libkwin.so from \"${KWIN_LIBRARY_PATH}\"")
    endif()
endif()

if (CMAKE_INSTALL_LIBDIR)
    install_targets("/${CMAKE_INSTALL_LIBDIR}" ${TARGET_NAME})
elseif (CMAKE_LIBRARY_OUTPUT_DIRECTORY)
    install(TARGETS ${TARGET_NAME} DESTINATION "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
else()
    install_targets("/lib" ${TARGET_NAME})
endif()

set(INCLUDE_OUTPUT_PATH "${CMAKE_INSTALL_PREFIX}/include/${PROJECT_NAME}")

install_files(
    "/include/${PROJECT_NAME}"
    FILES
    vtablehook.h
    common.h
    kwinutils.h
)

configure_file(${PROJECT_NAME}.pc.in ${PROJECT_NAME}.pc)
if (CMAKE_INSTALL_LIBDIR)
    install_files("/${CMAKE_INSTALL_LIBDIR}/pkgconfig" FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc)
elseif (CMAKE_LIBRARY_OUTPUT_DIRECTORY)
    install(FILES FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc DESTINATION "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/pkgconfig")
else()
    install_files("/lib/pkgconfig" FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc)
endif()
