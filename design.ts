let running_jobs: Job[] = []

let id_counter = 0

type DbMap = Map<string, Crontab>

interface Crontab {
  mtime: number
  jobs: Job[]
}

interface Job {
  id: number
  run_at: number
  line: string
  user: string
}

interface FFile {
  mtime: number
  fileName: string
  lines: string[]
}

interface DirConfig {
  mtime: number
  dirname: string
  files: FFile[]
}

function scan_jobs(_sys: DirConfig, usr: DirConfig, db_map: DbMap) {
  for (const file of usr.files) {
    let fileConfig = db_map.get(file.fileName)!

    if (!fileConfig) {
      db_map.set(file.fileName, {
        jobs: [],
        mtime: 0,
      })

      fileConfig = db_map.get(file.fileName)!
    }

    if (fileConfig.mtime < file.mtime) {
      const usr_jobs: Job[] = []

      for (const line of file.lines) {
        const job = make_job(line, file.fileName)
        usr_jobs.push(job)
      }

      fileConfig.jobs = usr_jobs
    }
  }

  return db_map
}

function run_job(job: Job, db_map: DbMap) {
  const crontab = db_map.get(job.user)!

  crontab.jobs = crontab.jobs.filter(x => x.id == job.id)

  running_jobs.push(job)
  console.log(running_jobs)
}

function make_job(line: string, user: string): Job {
  return {
    run_at: Date.now(),
    line,
    id: ++id_counter,
    user,
  }
}

function reap() {
  for (const running_job of running_jobs) {
    console.log(`job ${running_job.id} cleaned up`)
  }
  running_jobs = []
}

function queue_job(job: Job) {
  running_jobs.push(job)
  console.log(running_jobs)
}

const stime = 10
async function main(sys: DirConfig, usr: DirConfig) {
  while (true) {
    const db_map: DbMap = new Map()

    scan_jobs(sys, usr, db_map)

    console.log({ db_map })

    db_map.forEach(m => {
      console.log({ H: m.jobs })
    })

    const sleep_time = stime * 1000
    const curr = Date.now()
    console.log(`sleeping for ${sleep_time}ms`)

    await new Promise(r => setTimeout(r, sleep_time))

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

const f1: FFile = {
  fileName: 'yearly',
  lines: ['hi', 'world'],
  mtime: Date.now(),
}

const f2: FFile = {
  fileName: 'goldmund',
  lines: ['hsi', 'wosrld'],
  mtime: Date.now(),
}

const f3: FFile = {
  fileName: 'harumbi',
  lines: ['howod', 'howdy'],
  mtime: Date.now(),
}

const sys: DirConfig = {
  dirname: '/etc/cron.d',
  mtime: 0,
  files: [f1],
}

const usr: DirConfig = {
  dirname: '/var/crontab',
  mtime: 0,
  files: [f2, f3],
}

main(sys, usr)
