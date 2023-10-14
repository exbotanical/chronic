#!/usr/bin/env sh

dateval="$(date -u +"%Y-%m-%dT%H:%M:%S.%3NZ")"
echo "$dateval" >> "/tmp/cron.$1"
