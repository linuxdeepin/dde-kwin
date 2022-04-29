#!/bin/bash
export DISPLAY=:0.0
utdir=sanitizer
rm -r $utdir
rm -r ../$utdir
mkdir -p ../$utdir
cd ../$utdir

cmake -DCMAKE_SAFETYTEST_ARG="CMAKE_SAFETYTEST_ARG_ON" -DBUILD_TESTING=1 ..
make -j4

./tests/dde-kwin_test -o ut-report.txt

workdir=$(cd ../$(dirname $0)/$utdir; pwd)

mkdir -p report

lcov -c -i -d $workdir -o init.info

lcov -d $workdir -c -o ./coverage.info

lcov -a $workdir/init.info -a $workdir/coverage.info -o $workdir/total.info

lcov --remove $workdir/total.info '*/autotests/*' -o $workdir/total.info

genhtml -o ./ $workdir/total.info

exit 0
