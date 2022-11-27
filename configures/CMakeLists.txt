option(USE_KWIN_NO_SCALE "use deepin LD_PRELOAD kwin_no_scale" ON)

install(
    FILES
    kglobalshortcutsrc
    kwinrc
    deepin-kwinrulesrc
    klaunchrc
    kdeglobals
    DESTINATION
    "${CMAKE_INSTALL_SYSCONFDIR}/xdg"
)

install(
    FILES
    org.kde.kwin.splitmenu.display.json
    DESTINATION
    "${CMAKE_INSTALL_DATADIR}/dsg/configs/org.kde.kwin"
)

if (USE_KWIN_NO_SCALE)
    configure_file(kwin_no_scale.in kwin_no_scale)
    install(
	FILES ${CMAKE_CURRENT_BINARY_DIR}/kwin_no_scale
	DESTINATION "${CMAKE_INSTALL_BINDIR}"
	PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    )
endif()
