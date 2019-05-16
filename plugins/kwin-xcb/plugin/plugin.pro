# NOTE(sbw): 禁止语法树上的 vrp 优化，-O2/-O3 默认开启，会导致测试虚析构函数 HOOK 失败
QMAKE_CXXFLAGS_RELEASE += -fno-tree-vrp
include(../../../common.pri)
TARGET = $$PROJECT_NAME-xcb

PLUGIN_TYPE = platforms
PLUGIN_CLASS_NAME = KWinPlatformIntegration
!equals(TARGET, $$QT_DEFAULT_QPA_PLUGIN): PLUGIN_EXTENDS = -

QT       += core-private dbus widgets #xcb_qpa_lib-private
greaterThan(QT_MAJOR_VERSION, 4) {
    # Qt >= 5.8
    greaterThan(QT_MINOR_VERSION, 7): QT += gui-private
    else: QT += platformsupport-private

    # Qt >= 5.10
    greaterThan(QT_MINOR_VERSION, 9): QT += edid_support-private
}

TEMPLATE = lib
CONFIG += plugin c++11

SOURCES += \
    $$PWD/main.cpp \
    libkwinpreload.cpp

INCLUDEPATH += ../lib
LIBS += -L$$OUT_PWD/../lib -l$$PROJECT_NAME

CONFIG(debug, debug|release) {
    DEPENDPATH += $$PWD/../lib
    unix:QMAKE_RPATHDIR += $$OUT_PWD/../lib
}

DISTFILES += \
    $$PWD/$${TARGET}.json

isEmpty(INSTALL_PATH) {
    target.path = $$[QT_INSTALL_PLUGINS]/platforms
} else {
    target.path = $$INSTALL_PATH
}

DEFINES += PLATFORMS_PLUGIN_PATH=\\\"$$[QT_INSTALL_PLUGINS]/platforms\\\"

kwin_utils_dbus_interface.files = $$PWD/org.kde.kwin.KWinUtils.xml
kwin_utils_dbus_interface.path = $$PREFIX/share/dbus-1/interfaces

DBUS_ADAPTORS += $$PWD/org.kde.kwin.KWinUtils.xml

INSTALLS += target kwin_utils_dbus_interface

HEADERS += \
    libkwinpreload.h

!isEmpty(USE_DBUS_MENU) {
    DEFINES += USE_DBUS_MENU
}

!isEmpty(DISABLE_DXCB) {
    DEFINES += DISABLE_DXCB
}

TRANSLATIONS += $$PWD/translations/$${TARGET}.ts
DEFINES += TARGET_NAME=\\\"$${TARGET}\\\"

CONFIG(release, debug|release) {
    !system($$PWD/translate_generation.sh): error("Failed to generate translation")
}

translations.path = /usr/share/$${TARGET}/translations
translations.files = $$PWD/translations/*.qm

INSTALLS += translations
