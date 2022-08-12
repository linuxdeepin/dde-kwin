find_package(Qt5 CONFIG REQUIRED COMPONENTS Core Gui)
find_package(X11 REQUIRED)
find_package(KF5CoreAddons REQUIRED)
find_package(KF5WindowSystem REQUIRED)

set(INSTALL_PATH "${QT_INSTALL_PLUGINS}/kwin/effects/plugins")

if (ENABLE_BUILTIN_SCISSOR_WINDOW)
    add_subdirectory(scissor-window)
endif()

if (ENABLE_BUILTIN_BLUR)
    add_subdirectory(blur)
endif()

if (ENABLE_BUILTIN_MULTITASKING)
    add_subdirectory(multitasking)
endif()

if (ENABLE_BUILTIN_BLACK_SCREEN)
    add_subdirectory(black-screen)
endif()
