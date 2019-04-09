QT -= core
CONFIG -= qt
TEMPLATE = aux

include(../common.pri)

tabbox.files = \
    $$PWD/thumbnail_grid

tabbox.path = $$PREFIX/share/kwin/tabbox

INSTALLS += tabbox
