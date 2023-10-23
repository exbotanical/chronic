set -o errexit

RED='\033[0;31m'
GREEN='\033[1;32m'
YELLOW='\033[0;33m'

DEFAULT='\033[0m'

red () {
  printf "${RED}$@${DEFAULT}"
}

green () {
  printf "${GREEN}$@${DEFAULT}"
}

yellow () {
  printf "${YELLOW}$@${DEFAULT}"
}

for_each () {
  local fn=$1

  shift

  local -a arr=($@)

  for item in "${arr[@]}"; do
    $fn $item
  done
}

filter () {
  local fn=$1
  local suffix=$2
  local arg

  while read -r arg; do
    $fn $arg $suffix && echo $arg
  done
}

not_test_file () {
	local test=$1
  local suffix=$2
	local ret=0

  if [[ $test != *$suffix ]]; then
     return 1;
  fi

	for (( i=0; i < ${#SKIP_FILES[@]}; i++ )); do
		if [[ $test == ${SKIP_FILES[i]} ]]; then
			ret=1
			break
		fi
	done

	return $ret
}

quietly_kill () {
  kill $1 2>/dev/null
}


validate_timestamps () {
  local file="$1"
  local ival="$2"

  # Find the first timestamp in the file which is divisible by ival
  local start_time
  start_time=$(awk -v divisor="$ival" -F '[:T-]' '$6 % divisor == 0 {print; exit}' "$file")

  # If no such timestamp is found, exit with an error
  if [ -z "$start_time" ]; then
    echo "No timestamp divisible by $ival found in $file."
    echo 1
  fi

  local expected_time="$start_time"
  local next_time

  while IFS= read -r timestamp; do
    # Ensure the timestamp in the file matches the expected time
    [ "$timestamp" = "$expected_time" ] || {
      echo "Mismatch in $file. Expected $expected_time, found $timestamp"; echo 1;
    }

    # Calculate the next expected time
    next_time=$(date -d "$expected_time $ival minutes" +'%Y-%m-%dT%H:%M:%S')
    expected_time="$next_time"
  done < "$file"
  echo 0
}
