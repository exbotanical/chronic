#!/usr/bin/env sh
rm chronic && make && ./chronic
sleep 10
ps aux | grep './chronic' | awk '{ print $2 }'
