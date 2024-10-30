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

file(GLOB KWIN_DCONFIG_ITEMS CONFIGURE_DEPENDS *.json)

install(
    FILES
    ${KWIN_DCONFIG_ITEMS}
    DESTINATION
    "/usr/share/dsg/configs/org.kde.kwin"
)

install(
    FILES
    org.kde.kwin.decoration.titlebar.json
    DESTINATION
    "/usr/share/dsg/configs/org.kde.kwin.decoration"
)

install(
    FILES
    org.deepin.kwin.json
    DESTINATION
    "/usr/share/deepin-log-viewer/deepin-log.conf.d/"
)

if (USE_KWIN_NO_SCALE)
    configure_file(kwin_no_scale.in kwin_no_scale)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/kwin_no_scale DESTINATION bin PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
endif()
