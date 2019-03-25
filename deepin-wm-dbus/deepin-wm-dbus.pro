QT += core dbus KConfigCore KWindowSystem KGlobalAccel

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

DeepinWM.files = $$PWD/com_deepin_wm.xml

DBUS_ADAPTORS += DeepinWM

#INSTALLS += target
