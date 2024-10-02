#!/usr/bin/env bash

ROOT_DIR="$(dirname "$(readlink -f $BASH_SOURCE)")"
SETUP_PATH="$ROOT_DIR/../t/integ/utils/setup.bash"

. "$SETUP_PATH"
uname_1='narcissus'
uname_2='demian'
uname_3='isao'

setup_user_crondir

setup_user $uname_1
setup_user $uname_2

setup_user_crontab $uname_3

add_user_job $uname_1 1
add_user_job $uname_2 1
add_user_job $uname_3 1

add_user_job $uname_1 3
add_user_job $uname_1 5
add_user_job $uname_2 2

service postfix start
