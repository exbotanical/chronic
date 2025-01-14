ROOT_DIR="$(dirname "$(readlink -f $BASH_SOURCE)")"

# shellcheck source=utils/shpec_utils.bash
. "$ROOT_DIR/utils/shpec_utils.bash"
# shellcheck source=utils/test_utils.bash
. "$ROOT_DIR/utils/test_utils.bash"

SETUP_PATH="$ROOT_DIR/utils/setup.bash"

ONE_MIN_IN_SECS=60

describe 'crond run by non-root'
  # Needed for invoking chronic as another user
  chmod -R 777 .

  # Required - Docker doesn't save running processes so we need to start the mail server here
  service postfix start

  # shellcheck source=utils/setup.bash
  . "$SETUP_PATH"

  uname_1='narcissus'
  uname_2='demian'

  # Setup
  setup_user_crondir
  setup_sys_cron

  setup_user $uname_1
  setup_user $uname_2

  # Add 1 min job for narcissus
  add_user_job $uname_1 1
  # Add 1 min job for demian
  add_user_job $uname_2 1

  trap 'quietly_kill; teardown_user "$uname_1" 2>/dev/null; \
  teardown_user "$uname_2" 2>/dev/null; \
  teardown_user_crondir 2>/dev/null; \
  teardown_sys_cron 2>/dev/null;' EXIT

  su $uname_1 -c './chronic -L .logx'

  # Wait for an even minute minus 1 second so we catch the next run
  # 10# to force base10 and avoid `bash: 60 - 08: value too great for base (error token is "08")`
  wait_val=$((ONE_MIN_IN_SECS - 10#$(date +%S) - 1))
  echo "sleeping for $wait_val seconds..."
  sleep $wait_val

  it 'runs only jobs owned by the current user'
    ls /tmp
    assert equal $(compgen -G "/tmp/$uname_1.*" | wc -l) 1
    ls /tmp

    assert equal $(compgen -G "/tmp/$uname_2.*" | wc -l) 0
    assert equal $(compgen -G "/tmp/root.*" | wc -l) 0
  ti

end_describe
