option(USE_KWIN_NO_SCALE "use deepin LD_PRELOAD kwin_no_scale" ON)

if(ENABLE_BUILTIN_BLUR)
     set(ENABLE_BUILTIN_BLUR_STRING "false")
 else()
     set(ENABLE_BUILTIN_BLUR_STRING "true")
 endif()

configure_file(kwinrc.in kwinrc)

install(
    FILES
    kglobalshortcutsrc
    ${CMAKE_CURRENT_BINARY_DIR}/kwinrc
    kwinrulesrc
    klaunchrc
    kdeglobals
    DESTINATION
    "/etc/xdg"
)

if (USE_KWIN_NO_SCALE)
    configure_file(kwin_no_scale.in kwin_no_scale)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/kwin_no_scale
            DESTINATION bin
            PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
endif()
