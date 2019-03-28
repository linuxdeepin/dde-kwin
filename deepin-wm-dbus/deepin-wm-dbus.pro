QT += core dbus KConfigCore KWindowSystem KGlobalAccel

include($$PWD/../common.pri)

TARGET = deepin-wm-dbus
TEMPLATE = app

CONFIG += c++11 link_pkgconfig

PKGCONFIG += dtkwidget

DEFINES += QT_DEPRECATED_WARNINGS

HEADERS += \
    deepinwmfaker.h

SOURCES += \
    main.cpp \
    deepinwmfaker.cpp

deepin_wm_dbus_interface.files = $$PWD/com.deepin.wm.xml
deepin_wm_dbus_interface.path = $$PREFIX/share/dbus-1/interfaces

DBUS_ADAPTORS += $$PWD/com.deepin.wm.xml

deepin_wm_dbus_service.files = $$PWD/com.deepin.wm.service
deepin_wm_dbus_service.path = $$PREFIX/share/dbus-1/services

target.path = $$PREFIX/bin

INSTALLS += target deepin_wm_dbus_interface deepin_wm_dbus_service
