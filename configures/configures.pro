QT -= core
CONFIG -= qt
TEMPLATE = aux

config.files = $$PWD/kglobalshortcutsrc $$PWD/kwinrc
config.path = /etc/xdg

kwin_fake.files = $$PWD/kwin_no_scale
kwin_fake.path = /usr/bin

INSTALLS += config kwin_fake
