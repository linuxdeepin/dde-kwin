# Qt对cmake版本的最小要求
cmake_minimum_required(VERSION 3.13.0)

set(TARGET_PATH "${CMAKE_INSTALL_PREFIX}/bin")
set(TARGET_NAME "deepin-watermark-dbus")

# 项目命名
project(deepin-watermark-dbus)

# 有些项目会动态生成头文件，项目中需要引入它，因此需要将output目录也include进来
# 等同于INCLUDE_DIRECTORY(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Qt5 对C++版本推荐至少11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt特有的编译器需要打开，默认是关闭的
set(CMAKE_AUTOMOC ON) # Meta-Object Compiler
set(CMAKE_AUTORCC ON) # Resource Compiler
set(CMAKE_AUTOUIC ON) # User Interface Compiler

# 寻找Qt的库
# Qt5 COMPONENTS Widgets：寻找Qt库中的Widget模块
# REQUIRED： 意味着找不到报错并不会继续下去
find_package(Qt5 COMPONENTS Core Gui Widgets X11Extras DBus REQUIRED)
find_package(X11)
# 当前模块的头文件肯定要include进来
include_directories(./)

# 当前模块下的cpp跟CMakeList.txt在同级目录
#aux_source_directory(./ SRC_LIST)

set(SRC_LIST main.cpp utils.cpp deepinwatermark.cpp)

set(LIBS
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::X11Extras
    Qt5::DBus
    X11
    Xext
)

# 1. 将<input>文件里面的内容全部复制到<output>文件中
# 2. 根据参数规则,替换@VAR@或${VAR}变量
configure_file(com.deepin.watermark.service.in com.deepin.watermark.service)

install(
    FILES
    deepin-watermark-dbus.desktop
    DESTINATION
    "/etc/xdg/autostart"
)

install_files(
    "/share/dbus-1/services"
    FILES
    ${CMAKE_CURRENT_BINARY_DIR}/com.deepin.watermark.service
)

qt5_add_dbus_adaptor(
    watermark_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/com.deepin.watermark.xml
    deepinwatermark.h
    DeepinWatermark
)

add_executable(${PROJECT_NAME} ${SRC_LIST} ${watermark_SRCS})

# 链接所有其他模块到当前模块
target_link_libraries(${PROJECT_NAME} PRIVATE ${LIBS})

install_targets("/bin" ${PROJECT_NAME})
