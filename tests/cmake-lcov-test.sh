#!/bin/bash
export DISPLAY=:0.0
utdir=build-ut
rm -r $utdir
rm -r ../$utdir
mkdir ../$utdir
cd ../$utdir

cmake -DCMAKE_SAFETYTEST_ARG="CMAKE_SAFETYTEST_ARG_ON" ..
make -j4

./tests/dde-kwin_test -o ut-report.txt

workdir=$(cd ../$(dirname $0)/$utdir; pwd)

mkdir -p report

lcov -d $workdir -c -o ./report/coverage.info

lcov --extract ./report/coverage.info '*/multitasking/*' -o ./report/coverage.info

lcov --remove ./report/coverage.info '*/autotests/*' -o ./report/coverage.info

genhtml -o ./report ./report/coverage.info

exit 0
