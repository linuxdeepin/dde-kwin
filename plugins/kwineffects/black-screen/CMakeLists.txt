set(TARGET_NAME "black-screen")

find_package(KF5Config REQUIRED)
find_package(Qt5DBus REQUIRED)

add_library(
    ${TARGET_NAME} MODULE
    black-screen.cpp
    main.cpp
)

include_directories(../../platforms/lib)

target_include_directories(${TARGET_NAME} PUBLIC
    ${Qt5Core_INCLUDE_DIRS}
    ${Qt5DBus_INCLUDE_DIRS}
)

target_link_libraries(
    ${TARGET_NAME}
    PUBLIC
    Qt5::Core
    Qt5::Gui
    Qt5::DBus
    PRIVATE
    KF5::CoreAddons
    KF5::WindowSystem
    KF5::ConfigCore
)

if (NOT CMAKE_BUILD_TYPE MATCHES Debug)
  install(TARGETS ${TARGET_NAME} DESTINATION ${INSTALL_PATH})
endif ()
