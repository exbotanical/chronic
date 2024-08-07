1. Initialization
   1. Initialize logger
   2. Start daemon as background process
   3. Read crontab - `/etc/crontab` (sys) and `/var/spool/cron/crontabs` (usr)
      1. If daemon run as root, process all; else only current user
2. Main loop
   1. Check current time
   2. Compare to tasks listed in crontab
   3. Execute overdue tasks
   4. Sleep for *N*ms interval
   5. Repeat

3. Parsing (on start-up, then in watcher thread)
   1. Parse crontab line-by-line
      1. Strip comments
      2. Ignore empty lines
      3. Parse fields: minute, hour, day of month, month, day of week, command to run
      4. Store in job_t struct
   2. Watcher thread using inotify to process change events on the crontab (re-parsing)
      1. Use mutex on global job structs


---
Storing in job_t:
Priority queue (heap) to store tasks?
1. Parse crontabs and compute a timestamp for next execution time
2. At end of each iteration of main loop, sleep for a computed time to always have even minute


4. Check for Overdue Tasks
   1. Check if current time matches cron timing param
      1. If so, schedule task

5. Task Execution
   1. Spawn new process
   2. Run command
   3. Redirect outputs to logs/files
   4. In parent, `waitpid` on the child
   5. Record exit statuses and errors

6. Signal Handling
   1. Catch and handle signals
   2. SIGHUP -> re-read crontab
   3. SIGTERM -> terminate daemon

7. Cleanup Task
   1. Stop running tasks
   2. Close file descriptors
   3. flush logs

```
parse():
  for line in crontab:
    job = parse(line)
    job.nextExecution = computeTimestamp(job.pattern)
    queue.push(job)

do_main_loop():
    for:
    for job in queue:
      if job.nextExecution <= currentTime:
        execute(job)
        job.nextExecution = computeTimestamp(job.pattern)

    sleepDuration = computeSleep(currentTime)
    sleep(sleepDuration)

watcher_thread():
  for event in inotify.event:
    do_reparse_etc(event)

main():
  become_daemon()

  setup_logger()
  setup_signal_handlers()

  parse()

  pthread_init(watcher_thread)
  do_main_loop()
```

# concurrency and synchronization
- mutex in main and watcher threads
  - use granular locks (per-job to reduce contention)
- job states e.g. scheduled,running,complete

## alternative to inotify
1. set global db mtime to 0
2. every iter, scan crontabs
3. compare mtime
4. for each crontab file, create a meta tracking object and set the mtime

```ts
type Metadata {
  filePath: string
  crontabs: CrontabEntry[]
}

const crontabDir = '/etc/some/path';

for (const { file } of readdir(crontabDir)) {
  let metadata = getMetadata(file.path);
  if (!metadata) {
    metadata = createMetadata(file.path);
  }
  if (stat(file).mtime > metadata.mtime) {

    // setup crontabs
  } else {
    continue;
  }
}
```


## The New New Plan

```js
interface job_t {

}

interface User {

}

running_jobs = []

main() {
  while (true) {
    curr = now()
    db_map = {}

    // { root: [1, 2, 3, 4], goldmund: [1, 2]}
    // Note: db_map should actually be a linked list of linked lists?
    // Nah, we'll use HashMap<username, LinkedList<job_t>>
    scan_crontabs(sys, usr, db_map)

    db_map.entries().for_each(([user, job]) =>
      if curr == job.run_at {
        run_job(job, user)
      }
    )

    // { root: [1, 3], goldmund: [2]}
    running_jobs.for_each(j =>
      reap(j)
    )
  }
}

scan_crontabs(sys, usr, db_map) {

  for (file in sys.dirname) {
    sys_jobs = []

    // always runs the first time
    if (sys.mtime < file.mtime) {
      for (line in file) {
        job = new_job(line)
        sys_jobs.push(job)
      }
    }

    if (sys_jobs.length > 0) {
      if (db_map.has("root")) {
        db_map.get("root") = sys_jobs)
      } else {
        db_map.put("root", sys_jobs)
      }
    }
  }

  for (file in usr.dirname) {
    usr_jobs = []

    // always runs the first time
    if (usr.mtime < file.mtime) {
      for (line in file) {
        job = new_job(line)
        usr_jobs.push(job)
      }
    }

    if (usr_jobs.length > 0) {
      if (db_map.has("root")) {
        db_map.get("root") = usr_jobs)
      } else {
        db_map.put("root", usr_jobs)
      }
    }
  }

  return db_map
}

run_job(job, user) {
  running_jobs.push({
    id: job.id,
    ...blablabla
  })

  jobs.remove(job)

  run(job)
}

reap(j) {
  output = waitpid(j.pid)
  email_user(output, j)
}
```

---

1. Initialize
   1. Check lockfile
      1. if can open file
         1. if locked, fatal
         2. else, create lock
      2. else, fatal
   2. set locale?
   3. read cli args
   4. setup logger
   5. setup signal handling
   6. daemonize
