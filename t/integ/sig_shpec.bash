ROOT_DIR="$(dirname "$(readlink -f $BASH_SOURCE)")"

# shellcheck source=utils/shpec_utils.bash
. "$ROOT_DIR/utils/shpec_utils.bash"
# shellcheck source=utils/test_utils.bash
. "$ROOT_DIR/utils/test_utils.bash"

describe 'signal handling functionality'
  alias setup='./chronic -L .log'
  alias teardown='quietly_kill'

  it 'cleanly exits on SIGINT'
    kill -SIGINT $(get_chronic_pid)
    grep 'received a kill signal (2)' .log > /dev/null
    assert equal "$?" 0
  ti

  it 'cleanly exits on SIGQUIT'
    kill -SIGQUIT $(get_chronic_pid)
    grep 'received a kill signal (3)' .log > /dev/null
    assert equal "$?" 0
  ti

  it 'cleanly exits on SIGTERM'
    kill -SIGTERM $(get_chronic_pid)
    grep 'received a kill signal (15)' .log > /dev/null
    assert equal "$?" 0
  ti
end_describe
