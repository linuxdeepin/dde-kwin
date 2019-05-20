#!/bin/bash

DESKTOP_SOURCE_FILE=configures/kwin-wm-multitaskingview.desktop
DESKTOP_TS_DIR=translations/desktop/

/usr/bin/deepin-desktop-ts-convert desktop2ts $DESKTOP_SOURCE_FILE $DESKTOP_TS_DIR
