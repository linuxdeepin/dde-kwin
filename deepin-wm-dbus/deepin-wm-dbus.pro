QT += core dbus dtkcore KConfigCore KWindowSystem KGlobalAccel

include($$PWD/../common.pri)

TARGET = deepin-wm-dbus
TEMPLATE = app

CONFIG += c++11 link_pkgconfig

DEFINES += QT_DEPRECATED_WARNINGS

HEADERS += \
    deepinwmfaker.h

SOURCES += \
    main.cpp \
    deepinwmfaker.cpp

# 不兼容 deepin-wm 的接口
CONFIG(DISABLE_DEEPIN_WM) {
    # xml 中添加注释
    DEEPIN_WM_PREFIX = <!-- By "CONFIG+=DISABLE_DEEPIN_WM"
    DEEPIN_WM_SUFFIX = -->
    DEFINES += DISABLE_DEEPIN_WM
} else {
    PKGCONFIG += gsettings-qt
}

dbus_xml.input = $$PWD/com.deepin.wm.xml.in
dbus_xml.output = $$OUT_PWD/com.deepin.wm.xml

dbus_service.input = $$PWD/com.deepin.wm.service.in
dbus_service.output = $$OUT_PWD/com.deepin.wm.service

QMAKE_SUBSTITUTES += dbus_xml dbus_service
QMAKE_CLEAN += $${dbus_xml.output} $${dbus_service.output}

deepin_wm_dbus_interface.files = $${dbus_xml.output}
deepin_wm_dbus_interface.path = $$PREFIX/share/dbus-1/interfaces

DBUS_ADAPTORS += $${dbus_xml.output}

deepin_wm_dbus_service.files = $${dbus_service.output}
deepin_wm_dbus_service.path = $$PREFIX/share/dbus-1/services

target.path = $$PREFIX/bin

INSTALLS += target deepin_wm_dbus_interface deepin_wm_dbus_service
