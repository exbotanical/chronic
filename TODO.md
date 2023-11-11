# TODOs

- [ ] cleanup
- [ ] allow user to interpolate a `RELATIVE_DIR` in crontabs (so they can reference files relative to the crontab)
- [x] allow variables in crontab
- [ ] schedule and run overdue jobs
- [ ] job reporting
- [x] synchronize updates to crontab
- [ ] actually read the sys ~~and user crontabs~~
  - [ ] perms
- [ ] special crontab config support (toml or yaml?)
- [x] log to syslog
  - [ ] syslog levels?
- [x] integ testing
- [x] unit tests
- [ ] Ensure tests don't log to syslog or whatever (maybe a test sink?)
- [ ] handle signals and log getting closed or deleted
- [ ] daemon lock
- [ ] Allow log feature flags (fs logs, job logs, time logs, etc)
