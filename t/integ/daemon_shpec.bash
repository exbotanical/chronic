ROOT_DIR="$(dirname "$(readlink -f $BASH_SOURCE)")"

# shellcheck source=utils/shpec_utils.bash
. "$ROOT_DIR/utils/shpec_utils.bash"
# shellcheck source=utils/test_utils.bash
. "$ROOT_DIR/utils/test_utils.bash"

describe 'daemon functionality'
  alias setup='./chronic -L .log'
  alias teardown='quietly_kill'

  it 'fast fails when another instance is already running'
    ./chronic -L .log2
    assert grep "$(cat .log2)" 'crond: flock error'
  ti

  it 'releases the lock when it dies'
    quietly_kill
    ./chronic -L .log3
    assert grep "$(cat .log3)" 'started at'
  ti
end_describe
