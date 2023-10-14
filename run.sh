#!/usr/bin/env sh
make clean && make && ./chronic
ps aux | grep './chronic' |  grep -vE 'color|codium' | awk '{ print $2 }'
