include(../../../common.pri)
TARGET = $$PROJECT_NAME
TEMPLATE = lib
QT += x11extras KConfigCore KCoreAddons KWindowSystem
QT -= gui
CONFIG += create_pc create_prl no_install_prl
DEFINES += KWINDOWSYSTEM_NO_QWIDGET

SOURCES += \
    $$PWD/vtablehook.cpp \
    $$PWD/kwinutils.cpp
HEADERS += \
    $$PWD/vtablehook.h \
    $$PWD/common.h \
    $$PWD/kwinutils.h

target.path = $$LIB_INSTALL_DIR
includes.files = $$PWD/*.h
includes.path = $$PREFIX/include/$$PROJECT_NAME

QMAKE_PKGCONFIG_LIBDIR = $$target.path
QMAKE_PKGCONFIG_VERSION = $$VERSION
QMAKE_PKGCONFIG_DESTDIR = pkgconfig
QMAKE_PKGCONFIG_NAME = $$PROJECT_NAME
QMAKE_PKGCONFIG_DESCRIPTION = DDE KWin plugin library
QMAKE_PKGCONFIG_INCDIR = $$includes.path

INSTALLS += target includes
