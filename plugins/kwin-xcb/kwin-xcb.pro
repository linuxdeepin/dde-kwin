# NOTE(sbw): 禁止语法树上的 vrp 优化，-O2/-O3 默认开启，会导致测试虚析构函数 HOOK 失败
QMAKE_CXXFLAGS_RELEASE += -fno-tree-vrp

PLUGIN_TYPE = platforms
PLUGIN_CLASS_NAME = KWinPlatformIntegration
!equals(TARGET, $$QT_DEFAULT_QPA_PLUGIN): PLUGIN_EXTENDS = -

QT       += core-private #xcb_qpa_lib-private
greaterThan(QT_MAJOR_VERSION, 4) {
    # Qt >= 5.8
    greaterThan(QT_MINOR_VERSION, 7): QT += gui-private
    else: QT += platformsupport-private

    # Qt >= 5.10
    greaterThan(QT_MINOR_VERSION, 9): QT += edid_support-private
}

TEMPLATE = lib

isEmpty(VERSION) {
    isEmpty(VERSION): VERSION = $$system(git describe --tags --abbrev=0)
    VERSION = $$replace(VERSION, [^0-9.],)
    isEmpty(VERSION): VERSION = 0.0.1
}

CONFIG += plugin c++11

SOURCES += \
    $$PWD/main.cpp \
    $$PWD/vtablehook.cpp

HEADERS += \
    $$PWD/vtablehook.h

DISTFILES += \
    $$PWD/kwin-xcb.json

isEmpty(INSTALL_PATH) {
    target.path = $$[QT_INSTALL_PLUGINS]/platforms
} else {
    target.path = $$INSTALL_PATH
}

DEFINES += PLATFORMS_PLUGIN_PATH=\\\"$$[QT_INSTALL_PLUGINS]/platforms\\\"
INSTALLS += target
