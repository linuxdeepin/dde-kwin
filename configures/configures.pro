QT -= core
CONFIG -= qt
TEMPLATE = aux

config.files = \
    $$PWD/kglobalshortcutsrc \
    $$PWD/kwinrc \
    $$PWD/kwinrulesrc \
    $$PWD/klaunchrc \
    $$PWD/kdeglobals

config.path = /etc/xdg

include(../common.pri)

kwin_no_scale.input = $$PWD/kwin_no_scale.in
kwin_no_scale.output = $$OUT_PWD/kwin_no_scale
QMAKE_SUBSTITUTES += kwin_no_scale

kwin_fake.files = $${kwin_no_scale.output}
kwin_fake.path = $$PREFIX/bin

kwin_multitaskingview.files = $$PWD/kwin-wm-multitaskingview.desktop
kwin_multitaskingview.path = $$PREFIX/share/applications

INSTALLS += config kwin_fake kwin_multitaskingview
