#ifndef WORKLOAD_H
#define WORKLOAD_H

#include "config.h"
#include "crontab.h"

static dir_config sys_dir = {.is_root = true, .path = SYS_CRONTABS_DIR};
static dir_config sysapp_dir = {.is_root = true, .path = "/etc", .regex = "crontab"};  // TODO: tested?
static dir_config hourly_dir = {.is_root = true, .path = SYS_HOURLY_CRONTABS_DIR, .cadence = CADENCE_HOURLY};
static dir_config daily_dir = {.is_root = true, .path = SYS_DAILY_CRONTABS_DIR, .cadence = CADENCE_DAILY};
static dir_config weekly_dir = {.is_root = true, .path = SYS_WEEKLY_CRONTABS_DIR, .cadence = CADENCE_WEEKLY};
static dir_config monthly_dir = {.is_root = true, .path = SYS_MONTHLY_CRONTABS_DIR, .cadence = CADENCE_MONTHLY};

static dir_config usr_dir = {.is_root = false, .path = CRONTABS_DIR};

#define ALL_DIRS \
  &usr_dir, &sys_dir, &sysapp_dir, &hourly_dir, &daily_dir, &weekly_dir, &monthly_dir, NULL

#endif /* WORKLOAD_H */
