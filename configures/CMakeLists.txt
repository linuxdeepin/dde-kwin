configure_file(kwin_no_scale.in kwin_no_scale)

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

install_files("/bin" FILES ${CMAKE_CURRENT_BINARY_DIR}/kwin_no_scale)
