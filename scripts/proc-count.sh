#!/usr/bin/env sh
ps aux | grep 'defunct' | grep -v 'grep' | awk '{print $2}' | wc -l
