option(USE_KWIN_NO_SCALE "use deepin LD_PRELOAD kwin_no_scale" OFF) # deepin v25 don't need kwin_no_scale

install(
    FILES
    kglobalshortcutsrc
    kwinrc
    kwinrulesrc
    klaunchrc
    kdeglobals
    DESTINATION
    "${CMAKE_INSTALL_SYSCONFDIR}/xdg"
)

file(GLOB KWIN_DCONFIG_ITEMS CONFIGURE_DEPENDS *.json)

install(
    FILES
    ${KWIN_DCONFIG_ITEMS}
    DESTINATION
    "${CMAKE_INSTALL_DATADIR}/dsg/configs/org.kde.kwin"
)

install(
    FILES
    org.kde.kwin.decoration.titlebar.json
    DESTINATION
    "${CMAKE_INSTALL_DATADIR}/dsg/configs/org.kde.kwin.decoration"
)

install(
    FILES
    org.deepin.kwin.json
    DESTINATION
    "${CMAKE_INSTALL_DATADIR}/deepin-log-viewer/deepin-log.conf.d/"
)

if (USE_KWIN_NO_SCALE)
    configure_file(kwin_no_scale.in kwin_no_scale)
    install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/kwin_no_scale DESTINATION "${CMAKE_INSTALL_BINDIR}")
endif()
