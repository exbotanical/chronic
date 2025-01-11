ROOT_DIR="$(dirname "$(readlink -f $BASH_SOURCE)")"

# shellcheck source=utils/shpec_utils.bash
. "$ROOT_DIR/utils/shpec_utils.bash"
# shellcheck source=utils/test_utils.bash
. "$ROOT_DIR/utils/test_utils.bash"

SETUP_PATH="$ROOT_DIR/utils/setup.bash"

# Use when you're developing tests and running your own instance of chronic.
# This will prevent the test setup/teardown hooks from running and ruining your day.
DEVMODE=0

# TODO: Allow set proc name

sock_call () {
  echo "$*" | socat - UNIX-CONNECT:/tmp/chronic.sock
}

start_chronic () {
  (( $DEVMODE == 0 )) && {
    ./chronic -L .log
  }
}

stop_chronic () {
  (( $DEVMODE == 0 )) && {
    quietly_kill
  }
}

test_data_setup () {
  # dev mode is BYOD (bring your own data)
  (( $DEVMODE == 1 )) && return
  echo 'setting up test data...'
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

  add_user_job $uname_1 1
  add_user_job $uname_2 1

  trap 'quietly_kill; teardown_user "$uname_1" 2>/dev/null; \
  teardown_user "$uname_2" 2>/dev/null; \
  teardown_user_crondir 2>/dev/null; \
  teardown_sys_cron 2>/dev/null;' EXIT
}

test_data_setup

describe 'ipc API failure modes'
  start_chronic
  sleep 2

  it 'responds appropriately to invalid format'
    invalid_inputs=(
      ''
      '{'
      '}'
      '{"}'
      '['
      ']'
      '[]}'
      '[]'
      '\"command\": \"whatever\"'
    )

    for in in "${invalid_inputs[@]}"; do
      assert equal "$(jq -r '.error' <<< $(sock_call $in))" 'invalid format'
    done
  ti

  it 'responds appropriately to missing command'
    invalid_inputs=(
      '{}'
      '{"Command":""}'
      '{"": ""}'
    )

    for in in "${invalid_inputs[@]}"; do
      assert equal "$(jq -r '.error' <<< $(sock_call $in))" 'missing command'
    done
  ti

  it 'responds appropriately to unknown command'
    invalid_inputs=(
      '{"command":""}'
      '{"command": ""}'
      '{"command" : ""}'
      '{"command": "whatever"}'
    )

    for in in "${invalid_inputs[@]}"; do
      actual="$(sock_call $in)"
      expect="$(echo $in | jq -r '.command')"

      assert equal "$actual" "{\"error\":\"unknown command '$expect'\"}"
    done
  ti

  stop_chronic
end_describe

describe 'ipc API IPC_SHOW_INFO command'
  start_chronic
  sleep 2

  it 'displays correct daemon pid'
    # We'll repeat this because calling with slightly different format each time
    # helps us implicitly test that TPDGAF (the parser doesn't give a fuck).
    out="$(sock_call '{"command":"IPC_SHOW_INFO"}')"

    assert equal "$(jq -r '.pid' <<< $out)" "$(get_chronic_pid)"
  ti

  it 'displays correct version'
    version="$(grep '^PROGVERS *:=' Makefile | awk -F ':=' '{print $2}' | xargs)"
    out="$(sock_call '{"command":"IPC_SHOW_INFO"}')"

    assert equal "$(jq -r '.version' <<< $out)" "$version"
  ti

  it 'displays valid uptime'
    out="$(sock_call '{ "command" : "IPC_SHOW_INFO"}' )"
    actual="$(jq -r '.uptime' <<< $out)"

    assert egrep "$actual" "^0 days, 0 hours, [0-9][0-9]* minutes, [1-9][0-9]* seconds$"
  ti

  it 'displays valid started_at'
    today=$(date +%Y-%m-%d)
    expect="^${today} [0-2][0-9]:[0-5][0-9]:[0-5][0-9]$"
    out="$(sock_call '{ "command" : "IPC_SHOW_INFO"}' )"

    assert egrep "$(jq -r '.started_at' <<< $out)" "$expect"
  ti

  stop_chronic
end_describe

describe 'ipc API IPC_LIST_CRONTABS command'
  start_chronic
  sleep 5

  out="$(sock_call '{ "command" : "IPC_LIST_CRONTABS"}' 2>/dev/null | tr -d '\0')"

  it 'lists all crontabs'
    assert equal $(jq 'length' <<< "$out") 3
  ti

  it 'has all required fields on each record'
    required_fields=('id' 'cmd' 'schedule' 'owner' 'envp' 'next')

    for field in "${required_fields[@]}"; do
      missing_count=$(jq "[.[].$field] | map(select(. == null)) | length" <<< $out)
      assert equal "$missing_count" 0
    done
  ti

  stop_chronic
end_describe

describe 'ipc API IPC_LIST_JOBS command'
  start_chronic
  # Must be at least 1 iteration of the cron loop for this test to be deterministic.
  echo 'sleeping for 60 seconds...'
  sleep 60

  out="$(sock_call '{ "command" : "IPC_LIST_JOBS"}' 2>/dev/null | tr -d '\0')"

  it 'lists all jobs'
    assert gt "$(jq 'length' <<< "$out")" 0
  ti

  it 'has all required fields on each record'
    required_fields=('id' 'cmd' 'mailto' 'state' 'next')

    for field in "${required_fields[@]}"; do
      missing_count=$(jq "[.[].$field] | map(select(. == null)) | length" <<< $out)
      assert equal "$missing_count" 0
    done
  ti

  stop_chronic
end_describe
