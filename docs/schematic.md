1. Init main program
   1. Daemonize the current process
   2. Initialize the logger
   3. Create and acquire lockfile
   4. Setup signal handlers
   5. Initialize queues
2. Read crontabs -> Update DB
   1. Init new DB
   2. For each crondir...
   3. Get filenames
   4. For each filename
      1. Get corresponding crontab from old DB
      2. If no crontab in old DB, create and insert to new DB
      3. Else continue
   5. If crontab file modified, recreate
   6. Else, renew next timestamp
   7. Delete old DB
3. Init reap routine and enter main loop
4. Try to run jobs
   1. For every db entry (crontab)
   2. If crontab next matches now, run
   3. Run job:
      1. Push job into job queue
      2. Get shell and home dir
      3. Fork and setup
      4. Exec
5. In main loop we sleep. Reaper routine runs.
   1. For each job in queue
      1. If process done and job type cron, run mailer job
   2. For each job in mail queue
      1. reap

---

crontab db -> HashTable<CrontabPath, Crontab>

Crontab
  entries = CrontabEntry[]
