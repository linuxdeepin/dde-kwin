include(../../../common.pri)
TARGET = $$PROJECT_NAME
TEMPLATE = lib
QT += x11extras KConfigCore KCoreAddons KWindowSystem
QT -= gui
CONFIG += create_pc create_prl no_install_prl

greaterThan(QT.KWindowSystem.MINOR_VERSION, 45) {
    DEFINES += KWINDOWSYSTEM_NO_QWIDGET
} else {
    QT += widgets
}

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

!isEmpty(KWIN_VERSION) {
    message("kwin version $$KWIN_VERSION")
    ver_list = $$split(KWIN_VERSION, .)

    isEmpty(KWIN_VER_MAJ) {
        KWIN_VER_MAJ = $$first(ver_list)
    }
    isEmpty(KWIN_VER_MIN) {
        KWIN_VER_MIN = $$member(ver_list, 1, 1)
        isEmpty(KWIN_VER_MIN):KWIN_VER_MIN = 0
    }
    isEmpty(KWIN_VER_PAT) {
        KWIN_VER_PAT = $$member(ver_list, 2, 2)
        isEmpty(KWIN_VER_PAT):KWIN_VER_PAT = 0
    }
    isEmpty(KWIN_VER_BUI) {
        KWIN_VER_BUI = $$member(ver_list, 3, 3)
        isEmpty(KWIN_VER_BUI):KWIN_VER_BUI = 0
    }

    DEFINES += KWIN_VERSION_STR=\\\"$$KWIN_VERSION\\\" \
               KWIN_VERSION_MAJ=$$KWIN_VER_MAJ \
               KWIN_VERSION_MIN=$$KWIN_VER_MIN \
               KWIN_VERSION_PAT=$$KWIN_VER_PAT \
               KWIN_VERSION_BUI=$$KWIN_VER_BUI
}

!isEmpty(KWIN_LIBRARY_PATH) {
    # 强制链接libkwin.so，kwin开发库未提供 libkwin.so，需要构建脚本自行创建链接到 /usr/lib/.../libkwin.so.x.x.x 的软连接
    LIBS += -L$$KWIN_LIBRARY_PATH -lkwin
}
