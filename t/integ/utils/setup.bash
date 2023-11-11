#!/usr/bin/env bash

USER_CRONDIR='/var/spool/cron/crontabs'

mk_basic_cron_cmd () {
  local uname="$1"
  local ival="$2"

  echo "*/$ival * * * * echo \$(date -u +'%Y-%m-%dT%H:%M:%S') >> /tmp/$uname.${ival}min"
}

setup_user () {
  local uname="$1"

  useradd -m -s /bin/bash $uname && \
  setup_user_crontab $uname
}

add_user_job () {
  local uname="$1"
  local ival="$2"

  echo "$(mk_basic_cron_cmd $uname $ival)" >> "$USER_CRONDIR/$uname"
}

rm_user_job () {
  local uname="$1"
  local ival="$2"

  sed -i "/\*\/$ival \* \* \* \* echo/d" "$USER_CRONDIR/$uname"
}

teardown_user () {
  local uname="$1"

  # Still try to delete the user even if this fails
  teardown_user_crontab $uname

  userdel -r $uname && \
  rm /tmp/$uname*
}

setup_user_crondir () {
  mkdir -p "$USER_CRONDIR"
}

setup_user_crontab () {
  local uname="$1"

  touch "$USER_CRONDIR/$uname" && \
  chmod 600 "$USER_CRONDIR/$uname"
}

teardown_user_crontab () {
  local uname="$1"

  rm "$USER_CRONDIR/$uname"
}

teardown_user_crondir () {
  rmdir "$USER_CRONDIR"
}
