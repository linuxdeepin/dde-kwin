add_subdirectory(platforms)

option(ENABLE_KDECORATION "enable deepin kdecoration plugin" ON)

if (ENABLE_KDECORATION)
    add_subdirectory(kdecoration)
endif()

add_subdirectory(kwineffects)
