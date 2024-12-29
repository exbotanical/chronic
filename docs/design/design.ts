// Doing this helped me conceptualize how this whole thing would work
interface crontab {
  mtime: number
  jobs: job_t[]
}

interface job_t {
  id: number
  run_at: number
  line: string
  user: string
  status: number
}

interface FFile {
  mtime: number
  fileName: string
  lines: string[]
  hasChanged: boolean
}

interface dir_config {
  mtime: number
  dirname: string
  files: FFile[]
}

type DbMap = Map<string, crontab>

let mail_jobs: job_t[] = /* mmap() */ []
let running_jobs: job_t[] = []

let id_counter = 0

const f1: FFile = {
  fileName: 'yearly',
  lines: ['hi', 'world'],
  mtime: Date.now(),
  hasChanged: false,
}

const f2: FFile = {
  fileName: 'goldmund',
  lines: ['hsi', 'wosrld'],
  mtime: Date.now(),
  hasChanged: false,
}

const f3: FFile = {
  fileName: 'harumbi',
  lines: ['howod', 'howdy'],
  mtime: Date.now(),
  hasChanged: true,
}

const sys: dir_config = {
  dirname: '/etc/cron.d',
  mtime: 0,
  files: [f1],
}

const usr: dir_config = {
  dirname: '/var/crontab',
  mtime: 0,
  files: [f2, f3],
}

function scan_crontabs(_sys: dir_config, usr: dir_config, old_db: DbMap) {
  let new_db: DbMap = new Map()
  console.log({ INNER: old_db })
  for (const file of usr.files) {
    let fileConfig = old_db.get(file.fileName)

    if (!fileConfig) {
      fileConfig = {
        jobs: [],
        mtime: 0,
      }
    } else {
      // if mtimes equal, add old entry to new to carry over unchanged
    }

    new_db.set(file.fileName, fileConfig)

    if (fileConfig.mtime < file.mtime) {
      const usr_jobs: job_t[] = []

      for (const line of file.lines) {
        const job = new_job(line, file.fileName)
        usr_jobs.push(job)
      }

      fileConfig.jobs = usr_jobs
    }
  }

  return new_db
}

function run_job(job: job_t, db_map: DbMap) {
  // must keep in og db for carry overs
  running_jobs.push(job)
  console.log(running_jobs)
}

function new_job(line: string, user: string): job_t {
  return {
    run_at: Date.now(),
    line,
    id: ++id_counter,
    user,
    status: -Infinity,
  }
}

function reap() {
  for (const running_job of running_jobs) {
    // pretend status here means waitpid returned
    if (running_job.status == 0) {
      push_to_mail_jobs(running_job)
    }
  }
  running_jobs = []
}

function queue_job(job: job_t) {
  running_jobs.push(job)
  console.log(running_jobs)
}

const stime = 10
async function main(sys: dir_config, usr: dir_config) {
  let db_map: DbMap = new Map()

  while (true) {
    const sleep_time = stime * 1000
    const curr = Date.now()
    console.log(`sleeping for ${sleep_time}ms`)

    await new Promise(r => setTimeout(r, sleep_time))

    db_map = scan_crontabs(sys, usr, db_map)

    console.log({ db_map })

    db_map.forEach(m => {
      console.log({ H: m.jobs })
    })

    Array.from(db_map.values()).forEach(crontab => {
      crontab.jobs.forEach(job => {
        if (curr == job.run_at) {
          console.log(`RUNNING JOB: ${curr} == ${job.run_at}`)
          run_job(job, db_map)
        }
      })
    })

    reap()
  }
}

function push_to_mail_jobs(job: job_t) {
  mail_jobs.push(job)
}

const send_email = console.log
// Running in another process
function job_mailer() {
  while (true) {
    for (const s of mail_jobs) {
      send_email(`blablabla ${s}`)
    }

    mail_jobs = []
  }
}

main(sys, usr)
