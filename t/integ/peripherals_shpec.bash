ROOT_DIR="$(dirname "$(readlink -f $BASH_SOURCE)")"

# shellcheck source=utils/shpec_utils.bash
. "$ROOT_DIR/utils/shpec_utils.bash"
# shellcheck source=utils/test_utils.bash
. "$ROOT_DIR/utils/test_utils.bash"

# TODO: Pass as opt for dyn
DEFAULT_LOGPATH='/var/log/crond.log'
SYSLOG_PATH='/var/log/syslog'

describe 'peripherals functionality'
  it 'treats syslog and log file opts as mutually exclusive'
    err=$(./chronic -L .log -S >/dev/null)
    assert grep 'cannot set log file when syslog is being used' "$err"

    err=$(./chronic -S -L .log >/dev/null)
    assert grep 'cannot set syslog opt when log file is being used' "$err"
  ti

  it 'uses default log path when no option is set'
    rm "$DEFAULT_LOGPATH"
    ./chronic
    grep 'crond: running as root' "$DEFAULT_LOGPATH" > /dev/null
    assert equal "$?" 0
    quietly_kill
  ti


  it 'writes to syslog when initialized with the -S option'
    kill $(pgrep rsyslogd) 2>/dev/null
    rm "$SYSLOG_PATH" 2>/dev/null

    rsyslogd 2>/dev/null
    ./chronic -S
    sleep 1

    grep 'running as root' "$SYSLOG_PATH" > /dev/null
    assert equal "$?" 0

    quietly_kill
    kill $(pgrep rsyslogd)
    rm "$SYSLOG_PATH"
  ti
end_describe
