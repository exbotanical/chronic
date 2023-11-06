#!/usr/bin/env bash
IFS=$'\n'

TESTING_DIR=t/integ
UTILS_F=run_utils.bash

declare -a SKIP_FILES=(
  # 'main_shpec.bash'
)

run_test () {
	local file_name="$1"

  shpec "$TESTING_DIR/$file_name"
}

run () {
  ! [[ -f /.dockerenv ]] && {
    echo "Must run in container"
    exit 1
  }

  # It's a daemon; we don't need to bg it ourselves...
  ./chronic -L .log

  # This step is critical:
  # We cannot use `$!` because the PID will be for the program itself (which sets up the daemon).
  # We'll sleep for 1 second, then grab the PID of the daemon process.
  sleep 1
  pid="$(ps aux | grep './chronic' |  grep -vE 'grep' | awk '{ print $2 }')"

  trap 'quietly_kill "$pid"' EXIT

	declare -a tests=(
    $(ls $TESTING_DIR | filter not_test_file _shpec.bash)
  )

	for_each run_test ${tests[*]}
}

main () {
  kill $(pgrep integ_test) 2>/dev/null ||:

  run
}

# shellcheck source=utils/run_utils.bash
. "$(dirname "$(readlink -f "$BASH_SOURCE")")"/$UTILS_F

main $*
