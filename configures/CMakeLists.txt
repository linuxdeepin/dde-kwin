option(USE_KWIN_NO_SCALE "use deepin LD_PRELOAD kwin_no_scale" ON)

install(
    FILES
    kglobalshortcutsrc
    kwinrc
    kwinrulesrc
    klaunchrc
    kdeglobals
    DESTINATION
    "/etc/xdg"
)

install(
    FILES
    org.kde.kwin.splitmenu.display.json
    DESTINATION
    "/usr/share/dsg/configs/org.kde.kwin"
)

if (USE_KWIN_NO_SCALE)
    configure_file(kwin_no_scale.in kwin_no_scale)
    install_files("/bin" FILES ${CMAKE_CURRENT_BINARY_DIR}/kwin_no_scale)
endif()
