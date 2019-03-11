QT -= core
CONFIG -= qt
TEMPLATE = aux

config.files = $$PWD/kglobalshortcutsrc $$PWD/kwinrc
config.path = /etc/xdg

INSTALLS += config
