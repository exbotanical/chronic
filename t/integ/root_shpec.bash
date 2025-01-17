ROOT_DIR="$(dirname "$(readlink -f $BASH_SOURCE)")"

# shellcheck source=utils/shpec_utils.bash
. "$ROOT_DIR/utils/shpec_utils.bash"
# shellcheck source=utils/test_utils.bash
. "$ROOT_DIR/utils/test_utils.bash"

SETUP_PATH="$ROOT_DIR/utils/setup.bash"

ONE_MIN_IN_SECS=60

describe 'crond run by root'
  ./chronic -L .log

  # Required - Docker doesn't save running processes so we need to start the mail server here
  service postfix start

  # shellcheck source=utils/setup.bash
  . "$SETUP_PATH"

  uname_1='narcissus'
  uname_2='demian'
  uname_3='isao'

  trap 'teardown_user "$uname_1" 2>/dev/null; \
  teardown_user "$uname_2" 2>/dev/null; \
  teardown_user "$uname_3" 2>/dev/null; \
  teardown_user_crondir 2>/dev/null; \
  teardown_sys_cron 2>/dev/null;' EXIT

  # Wait for an even minute minus 1 second so we catch the next run
  # 10# to force base10 and avoid `bash: 60 - 08: value too great for base (error token is "08")`
  wait_val=$((ONE_MIN_IN_SECS - 10#$(date +%S) - 1))
  echo "sleeping for $wait_val seconds..."
  sleep $wait_val

  now="$(date -u +'%Y-%m-%dT%H:%M:%S')"
  # Add an additional minute because the daemon will not see any crontabs first pass and will sleep until the next
  start_min="$(date -d "$now 1secs" +'%M')"

  # Setup
  setup_user_crondir
  setup_sys_cron

  setup_user $uname_1
  setup_user $uname_2

  # Add iaso's crontab (iaso user does not exist)
  setup_user_crontab $uname_3

  # Add 1 min job for narcissus
  add_user_job $uname_1 1
  # Add 1 min job for demian
  add_user_job $uname_2 1
  # Add 1 min job for isao (which won't run yet)
  add_user_job $uname_3 1

  # Add 3 min job for narcissus
  add_user_job $uname_1 3
  # Add 5 min job for narcissus
  add_user_job $uname_1 5
  # Add 2 min job for demian
  add_user_job $uname_2 2

  # Wait 3 min
  wait_val=$((3 * ONE_MIN_IN_SECS))
  echo "sleeping for $wait_val seconds..."
  sleep $wait_val

  # Delete 1 min job for narcissus
  rm_user_job $uname_1 1
  # Delete isao file
  teardown_user_crontab $uname_3

  # Total of 5 min now
  wait_val=$((2 * ONE_MIN_IN_SECS))
  echo "sleeping for $wait_val seconds..."
  sleep $wait_val

  it 'runs all jobs'
    assert file_present "/tmp/$uname_1.1min"
    assert file_present "/tmp/$uname_1.3min"

    assert file_present "/tmp/$uname_2.1min"
    assert file_present "/tmp/$uname_2.2min"

    (( start_min % 5 > 0 )) && {
      assert file_present "/tmp/$uname_1.5min"
    }
  ti

  it 'skips jobs for invalid user crontab'
    assert file_absent "/tmp/$uname_3.*"
  ti

  it 'runs jobs at the proper cadence'
    # Always 3 per-minute executions for uname_1 (we delete the job after 3 min)
    assert equal "$(wc -l < "/tmp/$uname_1.1min")" 3

    # Always 1 - 2 executions at a 3 min cadence for uname_1
    assert gt "$(wc -l < "/tmp/$uname_1.3min")" 0
    assert lt "$(wc -l < "/tmp/$uname_1.3min")" 3

    # Always 2-3 executions at a 2 min cadence for uname_2
    assert gt "$(wc -l <  "/tmp/$uname_2.2min")" 1
    assert lt "$(wc -l <  "/tmp/$uname_2.2min")" 4

    # Validate that the timestamps are exact to the minute at which ea. job should have run
    assert equal $(validate_timestamps "/tmp/$uname_1.1min" 1) 0
    assert equal $(validate_timestamps "/tmp/$uname_1.3min" 3) 0

    assert equal $(validate_timestamps "/tmp/$uname_2.1min" 1) 0
    assert equal $(validate_timestamps "/tmp/$uname_2.2min" 2) 0

    # Always 1 if we started at a minute divisible by 5
    (( start_min % 5 > 0 )) && {
      assert equal "$(wc -l <  "/tmp/$uname_1.5min")" 1
      assert equal $(validate_timestamps "/tmp/$uname_1.5min" 5) 0
    }
  ti

  it 'sends user mail'
    uname_1_mail_count="$(cat $MAIL_DIR/$uname_1 | grep 'Subject:' | wc -l 2>/dev/null)"
    uname_2_mail_count="$(cat $MAIL_DIR/$uname_2 | grep 'Subject:' | wc -l 2>/dev/null)"

    assert gt $uname_1_mail_count 1
    assert gt $uname_2_mail_count 1

    # TODO: Assert on actual mail contents once we finalize the logic there
  ti

  quietly_kill
end_describe
