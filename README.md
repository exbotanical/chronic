# chronic

Very much in-progress. Right now I'm just trying to get the main algorithm working (scheduling is hard). Then, I'll clean up this code and start structuring everything properly.

## Next steps
- add mailer routine
  - I want this to be a subprocess that reads the job queue via a mmapped pointer
- setup sys cron handling
- add logic around per-user permissions
- add actual crontab bin


## Development

1. Make scripts executable `chmod -R u+x ./scripts`
2. Run boot-dev script `./scripts/boot-dev.sh`
