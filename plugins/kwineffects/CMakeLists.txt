find_package(Qt5 CONFIG REQUIRED COMPONENTS Core Gui)
find_package(KF5CoreAddons REQUIRED)
find_package(KF5WindowSystem REQUIRED)

set(INSTALL_PATH "${QT_INSTALL_PLUGINS}/kwin/effects/plugins")

option(ENABLE_BUILTIN_SCISSOR_WINDOW "enable deepin built-in scissor window plugin" ON)
option(ENABLE_BUILTIN_BLUR "enable deepin built-in blur plugin" ON)
option(ENABLE_BUILTIN_MULTITASKING "enable deepin built-in multitasking plugin" ON)
option(ENABLE_BUILTIN_BLACK_SCREEN "enable deepin built-in black screen plugin" ON)

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
