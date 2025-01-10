#!/usr/bin/env bash
IFS=$'\n'

TESTING_DIR=t/integ
UTILS_F=run_utils.bash

declare -a SKIP_FILES=(
  # 'daemon_shpec.bash'
  # 'ipc_shpec.bash'
  # 'root_shpec.bash'
  # 'user_shpec.bash'
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
