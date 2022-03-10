#!/bin/sh

cd ../
[ -d build-ut ] && rm -fr build-ut
mkdir -p build-ut/html
mkdir -p build-ut/report
[ -d build ] && rm -fr build
mkdir -p build

cd build/

# Sanitize for asan.log
######################
export DISPLAY=:0.0
export XDG_CURRENT_DESKTOP=Deepin
export QT_IM_MODULE=fcitx
cmake -DCMAKE_SAFETYTEST_ARG="CMAKE_SAFETYTEST_ARG_ON" ..
make -j4

./tests/dde-kwin_test > interlog_dde-kwin.log 2>&1
######################
cd -
echo "Uos123!!" | sudo -S mv build/interlog*.log ./build-ut/asan_dde-kwin.log
echo "Uos123!!" | sudo -S chmod 744 ./build-ut/asan_dde-kwin.log
# UT for index.html and ut-report.txt
cd tests
sh cmake-lcov-test.sh
cd -
mv sanitizer/report/index.html build-ut/html/cov_dde-kwin.html
mv sanitizer/ut-report.txt build-ut/report/report_dde-kwin.xml
