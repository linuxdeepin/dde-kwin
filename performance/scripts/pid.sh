sudo bpftrace -e 'usdt:/usr/local/lib/aarch64-linux-gnu/libkwin.so:abstract_client:create_pid { time("%D %H:%M:%S "); printf("%s  %d.\n",str(arg0),arg1); }'
