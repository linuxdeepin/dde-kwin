QT -= core
CONFIG -= qt
TEMPLATE = aux

include(../common.pri)

themes.files = $$PWD/deepin $$PWD/deepin-dark
themes.path = $$PREFIX/share/aurorae/themes

INSTALLS += themes
