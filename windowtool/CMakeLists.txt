set(PROJECT_NAME "dde-windowtool")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11 -Wall")

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH} )

find_package(X11 REQUIRED)

find_package(KF5Wayland REQUIRED)
# Find the library
find_package(PkgConfig REQUIRED)
find_package(Qt5 COMPONENTS
    Core
    Gui
    Widgets
REQUIRED)

find_package(Libdrm 2.4.62)
set_package_properties(Libdrm PROPERTIES TYPE OPTIONAL PURPOSE "Required for drm output on Wayland.")
set(HAVE_DRM FALSE)
if(Libdrm_FOUND)
    set(HAVE_DRM TRUE)
endif()

find_package(gbm)
set_package_properties(gbm PROPERTIES TYPE OPTIONAL PURPOSE "Required for egl output of drm backend.")
set(HAVE_GBM FALSE)
if(HAVE_DRM AND gbm_FOUND)
    set(HAVE_GBM TRUE)
endif()

if(HAVE_GBM)
  add_definitions("-DHAVE_GBM")
endif()

set(SC_FILES
    src/XcbWindowLoader.cpp
    src/WaylandWindowLoader.cpp
    src/ScreenRecorder.cpp
    src/WindowInfoLoader.cpp
)

add_library(${PROJECT_NAME} SHARED ${SC_FILES})

set_target_properties(
    ${PROJECT_NAME} PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

target_link_libraries(${PROJECT_NAME}  Qt5::Core Qt5::Gui Qt5::Widgets)
target_link_libraries(${PROJECT_NAME}  KF5::WaylandClient)
target_link_libraries(${PROJECT_NAME}  X11)
target_link_libraries(${PROJECT_NAME}  xcb)
target_link_libraries(${PROJECT_NAME}  Libdrm::Libdrm)
target_link_libraries(${PROJECT_NAME}  gbm::gbm)

if (NOT CMAKE_BUILD_TYPE MATCHES Debug)
  if (CMAKE_INSTALL_LIBDIR)
      install_targets("/${CMAKE_INSTALL_LIBDIR}" ${PROJECT_NAME})
  elseif (CMAKE_LIBRARY_OUTPUT_DIRECTORY)
      install(TARGETS ${PROJECT_NAME} DESTINATION "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
  else()
      install_targets("/lib" ${PROJECT_NAME})
  endif()

  set(SC_HEADERS
    src/WindowInfo.h
    src/XcbWindowLoader.h
    src/WaylandWindowLoader.h
    src/ScreenRecorder.h
    src/WindowInfoLoader.h
  )

  install(FILES
    ${SC_HEADERS}
    DESTINATION include/windowtool COMPONENT Devel
  )
endif ()

if (BUILD_TESTING)
    add_subdirectory(demo)
endif()
