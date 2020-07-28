#!/bin/bash

local=/usr/local/bin/performance/
bpftrace $local/scripts/event.bt > $1/.performance_event.log &
bpftrace $local/scripts/drm.bt 2 > $1/.performance_drm.log &
#bash $local/desktop_click_app.sh 2 > $1/.performance_desktop.log &

fun_name=$(objdump -tT /lib/aarch64-linux-gnu/libdde-file-manager.so.1 | grep OpenFileEventE | grep -n '[0-9]FileController' | awk '{print $7}' | sed -n '1p')
result="uprobe:/lib/aarch64-linux-gnu/libdde-file-manager.so.1:$fun_name { time(\"%D %H:%M:%S.%f \"); printf(\"Desktop click app.\n\"); }"
bpftrace -e "$result" 2 > $1/.performance_desktop.log &