# chronic

A modern UNIX cron daemon.

## Development

1. Make scripts executable `chmod -R u+x ./scripts`
2. Run boot-dev script `./scripts/boot-dev.sh`
3. Tests are in t/. Use `make test`.
   1. NOTE: Run the integ tests in Docker. Otherwise, the test harness might make a mess of your system. `./scripts/boot-dev.sh` will bootstrap the environment, then you can run `make integ_test`.
