set(PROJECT_NAME "dde-windowtool")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11 -Wall")

#Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

find_package(X11 REQUIRED)

find_package(KF5Wayland REQUIRED)
# Find the library
find_package(PkgConfig REQUIRED)
find_package(Qt5 COMPONENTS
    Core
REQUIRED)

set(SC_FILES
    src/XcbWindowLoader.cpp
    src/WindowInfoLoader.cpp
)

add_library(${PROJECT_NAME} MODULE ${SC_FILES})

target_link_libraries(${PROJECT_NAME}  Qt5::Core)
target_link_libraries(${PROJECT_NAME}  KF5::WaylandClient)
target_link_libraries(${PROJECT_NAME}  X11)
target_link_libraries(${PROJECT_NAME}  xcb)

install(TARGETS ${PROJECT_NAME} DESTINATION lib)

set(SC_HEADERS
  src/WindowInfo.h
  src/XcbWindowLoader.h
  src/WindowInfoLoader.h
)

install(FILES
  ${SC_HEADERS}
  DESTINATION include/windowtool COMPONENT Devel
)