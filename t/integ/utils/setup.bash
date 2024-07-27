#!/usr/bin/env bash

SYS_CRONDIR='/etc/cron.d'
USER_CRONDIR='/var/spool/cron/crontabs'
MAIL_DIR='/var/spool/mail'

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
  rm /tmp/$uname* && \
  rm $MAIL_DIR/$uname
}

setup_user_crondir () {
  mkdir -p "$USER_CRONDIR"
}

setup_user_crontab () {
  local uname="$1"
  local f="$USER_CRONDIR/$uname"

  touch "$f" && \
  chmod 600 "$f" && \
  chown "$uname:$uname" "$f" 2> /dev/null
}

teardown_user_crontab () {
  local uname="$1"

  rm "$USER_CRONDIR/$uname"
}

teardown_user_crondir () {
  rmdir "$USER_CRONDIR"
}

setup_sys_cron () {
  mkdir -p "$SYS_CRONDIR"
  touch "$SYS_CRONDIR/root"
  chmod 600 "$SYS_CRONDIR/root"
  echo "$(mk_basic_cron_cmd root 1)" >> "$SYS_CRONDIR/root"
}

teardown_sys_cron () {
  rm -rf "$SYS_CRONDIR"
  rm /tmp/root*
}
