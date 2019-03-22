QT -= core
CONFIG -= qt
TEMPLATE = aux

include(../common.pri)

script.files = \
    $$PWD/screenedge/runcommandaction \
    $$PWD/screenedge/closewindowaction \
    $$PWD/ddeshortcuts

script.path = $$PREFIX/share/kwin/scripts

INSTALLS += script
