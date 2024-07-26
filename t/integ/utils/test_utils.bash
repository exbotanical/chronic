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

get_chronic_pid () {
  ps aux | grep './chronic' |  grep -vE 'grep' | awk '{ print $2 }'
}

quietly_kill () {
  pid="$(get_chronic_pid)"
  [[ -n $pid ]] && {
    kill $pid 2>/dev/null
  }
}
