set(CMAKE_SAFETYTEST "${CMAKE_SAFETYTEST_ARG}")
if(CMAKE_SAFETYTEST STREQUAL "")
    set(CMAKE_SAFETYTEST "CMAKE_SAFETYTEST_ARG_OFF")
endif()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D${CMAKE_SAFETYTEST}")
if(CMAKE_SAFETYTEST STREQUAL "CMAKE_SAFETYTEST_ARG_ON")
  #安全测试选项
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -fsanitize=undefined,address -O2")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -fsanitize=undefined,address -O2")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -fprofile-arcs -ftest-coverage")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -fprofile-arcs -ftest-coverage")

set(PROJECT_NAME_TEST
    ${PROJECT_NAME}_test)

find_package(Qt5Test REQUIRED)
find_package(Qt5 CONFIG REQUIRED COMPONENTS Core Gui )
#find_package(Qt5DBus REQUIRED)
find_package(KF5CoreAddons REQUIRED)
find_package(KF5WindowSystem REQUIRED)
find_package(KF5Config REQUIRED)

find_package(Qt5Quick REQUIRED)
find_package(Qt5QuickWidgets REQUIRED)
find_package(KF5GlobalAccel REQUIRED)
find_package(KF5I18n REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(Qt5LinguistTools REQUIRED)
find_package(Qt5X11Extras REQUIRED)

set( test
    ut_mulitask_model.cpp
    ../plugins/kwineffects/multitasking/multitasking_model.cpp
)

add_executable(${PROJECT_NAME_TEST} ${test})

target_link_libraries(
    ${PROJECT_NAME_TEST}
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Quick
    Qt5::QuickWidgets
    Qt5::Test
    Qt5::DBus
    Qt5::X11Extras
    KF5::ConfigCore
    KF5::CoreAddons
    KF5::WindowSystem
    KF5::GlobalAccel
    KF5::I18n
    Qt5::X11Extras
    xcb
    kwineffects
)

include_directories(../plugins/platforms/lib)

add_test(NAME dde-test COMMAND ${PROJECT_NAME_TEST})
add_custom_target(test
    COMMAND echo " =================== TEST BEGIN ==================== "
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMAND ${CMAKE_BINARY_DIR}/tests/${PROJECT_NAME_TEST}
    COMMAND echo " =================== TEST END ==================== "
)

add_dependencies(test ${PROJECT_NAME_TEST})
