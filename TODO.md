# TODOs

- cleanup
- allow user to interpolate a `RELATIVE_DIR` in crontabs (so they can reference files relative to the crontab)
- allow variables in crontab
- schedule and run overdue jobs
- job reporting
- synchronize updates to crontab
- actually read the sys and user crontabs
  - perms
- special crontab config support (toml or yaml?)
- log to syslog
- integ testing
- unit tests
- Ensure tests don't log to syslog or whatever (maybe a test sink?)
- handle signals and log getting closed or deleted
