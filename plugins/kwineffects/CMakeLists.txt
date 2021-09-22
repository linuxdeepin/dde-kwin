find_package(Qt5 CONFIG REQUIRED COMPONENTS Core Gui)
find_package(KF5CoreAddons REQUIRED)
find_package(KF5WindowSystem REQUIRED)
find_package(X11 REQUIRED)

set(INSTALL_PATH "${QT_INSTALL_PLUGINS}/kwin/effects/plugins")

add_subdirectory(scissor-window)
add_subdirectory(blur)
add_subdirectory(multitasking)
