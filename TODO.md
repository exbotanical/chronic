# TODOs

- [x] cleanup
- [x] ~~allow user to interpolate a `RELATIVE_DIR` in crontabs (so they can reference files relative to the crontab)~~
- [x] allow variables in crontab
- [x] job reporting
- [x] figure out better logging
- [x] synchronize updates to crontab
- [x] actually read the sys ~~and user crontabs~~
  - [x] perms
- [x] ~~log to syslog~~
  - [x] ~~syslog levels?~~
- [x] Allow loglogloglogloglog feature flags (fs logs, job logs, time logs, etc)
- [x] integ testing
- [x] unit tests
- [x] ~~Ensure tests don't log to syslog or whatever (maybe a test sink?)~~
- [x] handle signals
  - [x] and log getting closed or deleted (or segfault and anything else)
- [x] daemon lock
- [x] kill chronic anywhere integ test stops (e.g. TRAP)
- [x] ~~crontab program~~ no point - just use a text editor!
- [x] Fix all gcc warnings/errors and enable strict mode
- [x] all ts should be UTC
- [x] default log file
- [x] daily, weekly, etc
- [x] ~~detect shebang~~
- [x] format mail
  - [ ] test
- [ ] IPC features
  - [ ] print info for job <id>
  - [x] ~~print info for crontab <id>~~
  - [ ] pause job/crontab <id>
- [ ] validate permissions e.g. child job exec
- [ ] schedule and run overdue jobs
- [ ] special crontab config support (toml or yaml?)
- [ ] Store all jobs sorted by timestamp, then perform binary search when we want to execute. Enable this feature at runtime when numjobs > N.
- [ ] os compat
- [ ] load test
- [ ] ensure time is synchronized
- [ ] Name processes
- [ ] Use dir mtime - store get_filenames result and only ls dir if it changes
- [ ] Make all APIs consistent (e.g. return new string or always accept the dest as input)
- [ ] Document all behaviors and usage patterns; complete manpage
- [ ] Perf improvements
- [ ] Update Makefile template w/manpage and install updates


We make tradeoffs between runtime op/main loop work and startup work (loading the crontabs).
