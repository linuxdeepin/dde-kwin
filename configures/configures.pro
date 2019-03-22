QT -= core
CONFIG -= qt
TEMPLATE = aux

config.files = \
    $$PWD/kglobalshortcutsrc \
    $$PWD/kwinrc \
    $$PWD/kwinrulesrc

config.path = /etc/xdg

include(../common.pri)

kwin_fake.files = $$PWD/kwin_no_scale
kwin_fake.path = $$PREFIX/bin

INSTALLS += config kwin_fake
