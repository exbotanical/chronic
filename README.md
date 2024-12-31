# chronic

A modern UNIX cron daemon.

Features:
- Email reports on job completion.
- Runs jobs for multiple users. Assumes the appropriate user ident for each.
- Can run in root mode to execute system cron jobs
- Efficient, lazy updates. New crontabs are synced and scheduled within 60 seconds.
- Domain socket-based API for querying crontabs and job states.
- Atomic daemon with singleton support and signal-handling.
- Logging.
- Supports crontabs with environment variables.

## Development

1. Make scripts executable `chmod -R u+x ./scripts`
2. Run boot-dev script `./scripts/boot-dev.sh`
3. Tests are in t/. Use `make test`.
   1. NOTE: Run the integ tests in Docker. Otherwise, the test harness might make a mess of your system. `./scripts/boot-dev.sh` will bootstrap the environment, then you can run `make integ_test`.
