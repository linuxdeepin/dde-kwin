#!/bin/bash
# this file is used to auto-generate .qm file from .ts file.
# author: shibowen at linuxdeepin.com

EXECUTE_PATH=$(cd `dirname $0`; pwd)
ts_list=(`ls $EXECUTE_PATH/translations/*.ts`)

for ts in "${ts_list[@]}"
do
    printf "\nprocess ${ts}\n"
    lrelease "${ts}"
done
