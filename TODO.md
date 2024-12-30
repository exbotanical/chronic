# TODOs

- [x] cleanup
- [ ] allow user to interpolate a `RELATIVE_DIR` in crontabs (so they can reference files relative to the crontab)
- [x] allow variables in crontab
- [ ] schedule and run overdue jobs
- [ ] job reporting
- [x] synchronize updates to crontab
- [x] actually read the sys ~~and user crontabs~~
  - [x] perms
- [ ] special crontab config support (toml or yaml?)
- [x] ~~log to syslog~~
  - [x] ~~syslog levels?~~
- [ ] Allow log feature flags (fs logs, job logs, time logs, etc)
- [x] integ testing
- [x] unit tests
- [x] ~~Ensure tests don't log to syslog or whatever (maybe a test sink?)~~
- [x] handle signals
  - [ ] and log getting closed or deleted
- [x] daemon lock
- [x] kill chronic anywhere integ test stops (e.g. TRAP)
- [x] ~~crontab program~~ no point - just use a text editor!
- [ ] validate permissions e.g. child job exec
- [x] format mail
  - [ ] test
- [ ] Fix all gcc warnings/errors and enable strict mode
- [ ] Store all jobs sorted by timestamp, then perform binary search when we want to execute. Enable this feature at runtime when numjobs > N.


We make tradeoffs between runtime op/main loop work and startup work (loading the crontabs).
