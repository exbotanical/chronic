ROOT_DIR="$(dirname "$(readlink -f $BASH_SOURCE)")"

# shellcheck source=utils/shpec_utils.bash
. "$ROOT_DIR/utils/shpec_utils.bash"
# shellcheck source=utils/test_utils.bash
. "$ROOT_DIR/utils/test_utils.bash"

describe 'peripherals functionality'
  it 'treats syslog and log file opts as mutually exclusive'
    err=$(./chronic -L .log -S >/dev/null)
    assert grep 'cannot set log file when syslog is being used' "$err"

    err=$(./chronic -S -L .log >/dev/null)
    assert grep 'cannot set syslog opt when log file is being used' "$err"
  ti

  it 'writes to syslog when initialized with the -L option'
    rsyslog
    ./chronic -S
    cat /var/log/syslog
  ti
end_describe
