sudo bpftrace -e 'usdt:/usr/local/lib/aarch64-linux-gnu/libkwin.so:libinput:keyboard {  printf("%d %d %d %s.\n",arg0,arg1,arg2,str(arg3)); }' 2> ../logs/record.log
