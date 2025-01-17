/*
 * Copyright 2015, alex at staticlibs.net
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * File:   ccronexpr.c
 * Author: alex
 *
 * Created on February 24, 2015, 9:35 AM
 */

#include "ccronexpr.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CRON_MAX_SECONDS       60
#define CRON_MAX_LEAP_SECONDS  2
#define CRON_MAX_MINUTES       60
#define CRON_MAX_HOURS         24
#define CRON_MAX_DAYS_OF_MONTH 32
#define CRON_MAX_DAYS_OF_WEEK  7
#define CRON_MAX_MONTHS        12
#define CRON_MIN_YEARS         1970
#define CRON_MAX_YEARS         2200
#define CRON_MAX_YEARS_DIFF    4

#define YEAR_OFFSET            1900
#define DAY_SECONDS            24 * 60 * 60
#define WEEK_DAYS              7

#define CRON_CF_SECOND         0
#define CRON_CF_MINUTE         1
#define CRON_CF_HOUR_OF_DAY    2
#define CRON_CF_DAY_OF_WEEK    3
#define CRON_CF_DAY_OF_MONTH   4
#define CRON_CF_MONTH          5
#define CRON_CF_YEAR           6
#define CRON_CF_NEXT           7

#define CRON_CF_ARR_LEN        7

static const char* const DAYS_ARR[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
#define CRON_DAYS_ARR_LEN 7
static const char* const MONTHS_ARR[] = {"FOO", "JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
#define CRON_MONTHS_ARR_LEN       13

#define CRON_MAX_STR_LEN_TO_SPLIT 256
#define CRON_SIZE_STRING_MAX_LEN  20

#ifndef CRON_TEST_MALLOC
#  define cron_malloc(x) malloc(x)
#  define cron_free(x)   free(x)
#else  /* CRON_TEST_MALLOC */
void* cron_malloc(size_t n);
void  cron_free(void* p);
#endif /* CRON_TEST_MALLOC */

/**
 * Time functions from standard library.
 * This part defines: cron_mktime: create time_t from tm
 *                    cron_time: create tm from time_t
 */

/* forward declarations for platforms that may need them */
/* can be hidden in time.h */
#if !defined(_WIN32) && !defined(__AVR__) && !defined(ESP8266) && !defined(ESP_PLATFORM) && !defined(ANDROID) && !defined(TARGET_LIKE_MBED)
struct tm* gmtime_r(const time_t* timep, struct tm* result);
time_t     timegm(struct tm* __tp);
struct tm* localtime_r(const time_t* timep, struct tm* result);
#endif /* PLEASE CHECK _WIN32 AND ANDROID NEEDS FOR THESE DECLARATIONS */
#ifdef __MINGW32__
/* To avoid warning when building with mingw */
time_t _mkgmtime(struct tm* tm);
#endif /* __MINGW32__ */

/* function definitions */
#ifndef CRON_USE_LOCAL_TIME

static time_t
cron_mktime_gm (struct tm* tm) {
#  if defined(_WIN32)
  /* http://stackoverflow.com/a/22557778 */
  return _mkgmtime(tm);
#  elif defined(__AVR__)
  /* https://www.nongnu.org/avr-libc/user-manual/group__avr__time.html */
  return mk_gmtime(tm);
#  elif defined(ESP8266) || defined(ESP_PLATFORM) || defined(TARGET_LIKE_MBED)

#    error "timegm() is not supported on the ESP platform, please use this library with CRON_USE_LOCAL_TIME"

#  elif defined(ANDROID) && !defined(__LP64__)
  /* https://github.com/adobe/chromium/blob/cfe5bf0b51b1f6b9fe239c2a3c2f2364da9967d7/base/os_compat_android.cc#L20
   */
  static const time_t kTimeMax = ~(1L << (sizeof(time_t) * CHAR_BIT - 1));
  static const time_t kTimeMin = (1L << (sizeof(time_t) * CHAR_BIT - 1));
  time64_t            result   = timegm64(tm);
  if (result < kTimeMin || result > kTimeMax) {
    return -1;
  }
  return result;
#  else
  return timegm(tm);
#  endif
}

static struct tm*
cron_time_gm (time_t* date, struct tm* out) {
#  if defined(__MINGW32__)
  (void)(out); /* To avoid unused warning */
  return gmtime(date);
#  elif defined(_WIN32)
  errno_t err = gmtime_s(out, date);
  return 0 == err ? out : NULL;
#  elif defined(__AVR__)
  /* https://www.nongnu.org/avr-libc/user-manual/group__avr__time.html */
  gmtime_r(date, out);
  return out;
#  else
  return gmtime_r(date, out);
#  endif
}

#else

static time_t
cron_mktime_local (struct tm* tm) {
  tm->tm_isdst = -1;
  return mktime(tm);
}

static struct tm*
cron_time_local (time_t* date, struct tm* out) {
#  if defined(_WIN32)
  errno_t err = localtime_s(out, date);
  return 0 == err ? out : NULL;
#  elif defined(__AVR__)
  /* https://www.nongnu.org/avr-libc/user-manual/group__avr__time.html */
  localtime_r(date, out);
  return out;
#  else
  return localtime_r(date, out);
#  endif
}

#endif

/* Defining 'cron_' time functions to use use UTC (default) or local time */
#ifndef CRON_USE_LOCAL_TIME
time_t
cron_mktime (struct tm* tm) {
  return cron_mktime_gm(tm);
}

struct tm*
cron_time (time_t* date, struct tm* out) {
  return cron_time_gm(date, out);
}

#else  /* CRON_USE_LOCAL_TIME */
time_t
cron_mktime (struct tm* tm) {
  return cron_mktime_local(tm);
}

struct tm*
cron_time (time_t* date, struct tm* out) {
  return cron_time_local(date, out);
}

#endif /* CRON_USE_LOCAL_TIME */

/**
 * Functions.
 */

#define GET_BYTE(idx) (uint8_t)(idx / 8)
#define GET_BIT(idx)  (uint8_t)(idx % 8)

void
cron_set_bit (uint8_t* rbyte, int idx) {
  rbyte[GET_BYTE(idx)] |= (uint8_t)(1 << GET_BIT(idx));
}

void
cron_del_bit (uint8_t* rbyte, int idx) {
  rbyte[GET_BYTE(idx)] &= (uint8_t)~(1 << GET_BIT(idx));
}

uint8_t
cron_get_bit (const uint8_t* rbyte, int idx) {
  return (rbyte[GET_BYTE(idx)] & (1 << GET_BIT(idx))) ? 1 : 0;
}

static void
free_splitted (char** splitted, size_t len) {
  size_t i;
  if (!splitted) {
    return;
  }
  for (i = 0; i < len; i++) {
    if (splitted[i]) {
      cron_free(splitted[i]);
    }
  }
  cron_free(splitted);
}

static char*
strdupl (const char* str, size_t len) {
  char* res = NULL;
  if (!str) {
    return NULL;
  }
  res = (char*)cron_malloc(len + 1);
  if (!res) {
    return NULL;
  }
  memcpy(res, str, len);
  res[len] = '\0';
  return res;
}

static int
next_set_bit (uint8_t* bits, int max, int from_index, int* notfound) {
  int i;
  if (!bits) {
    *notfound = 1;
    return 0;
  }
  for (i = from_index; i < max; i++) {
    if (cron_get_bit(bits, i)) {
      return i;
    }
  }
  *notfound = 1;
  return 0;
}

static int
add_to_field (struct tm* calendar, int field, int val) {
  time_t res = 0;
  if (!calendar || -1 == field) {
    return 1;
  }
  switch (field) {
    case CRON_CF_SECOND: calendar->tm_sec = calendar->tm_sec + val; break;
    case CRON_CF_MINUTE: calendar->tm_min = calendar->tm_min + val; break;
    case CRON_CF_HOUR_OF_DAY:
      calendar->tm_hour = calendar->tm_hour + val;
      break;
    case CRON_CF_DAY_OF_WEEK: /* mkgmtime ignores this field */
    case CRON_CF_DAY_OF_MONTH:
      calendar->tm_mday = calendar->tm_mday + val;
      break;
    case CRON_CF_MONTH: calendar->tm_mon = calendar->tm_mon + val; break;
    case CRON_CF_YEAR: calendar->tm_year = calendar->tm_year + val; break;
    default: return 1; /* unknown field */
  }
  res = cron_mktime(calendar);
  return CRON_INVALID_INSTANT == res ? 1 : 0;
}

/**
 * Reset the calendar setting all the fields provided to zero.
 */
static int
reset_min (struct tm* calendar, int field) {
  time_t res = 0;
  if (!calendar || -1 == field) {
    return 1;
  }
  switch (field) {
    case CRON_CF_SECOND: calendar->tm_sec = 0; break;
    case CRON_CF_MINUTE: calendar->tm_min = 0; break;
    case CRON_CF_HOUR_OF_DAY: calendar->tm_hour = 0; break;
    case CRON_CF_DAY_OF_WEEK: calendar->tm_wday = 0; break;
    case CRON_CF_DAY_OF_MONTH: calendar->tm_mday = 1; break;
    case CRON_CF_MONTH: calendar->tm_mon = 0; break;
    case CRON_CF_YEAR: calendar->tm_year = 0; break;
    default: return 1; /* unknown field */
  }
  res = cron_mktime(calendar);
  return CRON_INVALID_INSTANT == res ? 1 : 0;
}

static int
last_day_of_month (int month, int year) {
  struct tm cal;
  time_t    t;
  memset(&cal, 0, sizeof(struct tm));
  cal.tm_mon  = month + 1;
  cal.tm_year = year;
  t           = cron_mktime(&cal);
  return cron_time(&t, &cal)->tm_mday;
}

static int
last_weekday_of_month (int month, int year) {
  struct tm cal;
  time_t    t;
  memset(&cal, 0, sizeof(struct tm));
  cal.tm_mon  = month + 1; /* next month */
  cal.tm_year = year;      /* years since 1900 */
  t           = cron_mktime(&cal);

  /* If the last day of the month is a Saturday (6) or Sunday (0), decrement the
   * day. But it is shifted to (5) and (6). */
  while (cron_time(&t, &cal)->tm_wday == 6 || cron_time(&t, &cal)->tm_wday == 0) {
    t -= DAY_SECONDS; /* subtract a day */
  }
  return cron_time(&t, &cal)->tm_mday;
}

static int
closest_weekday (int day_of_month, int month, int year) {
  struct tm cal;
  time_t    t;
  int       wday;
  memset(&cal, 0, sizeof(struct tm));
  cal.tm_mon  = month; /* given month */
  cal.tm_mday = day_of_month + 1;
  cal.tm_year = year;  /* years since 1900 */
  t           = cron_mktime(&cal);

  wday        = cron_time(&t, &cal)->tm_wday;

  /* If it's a Sunday */
  if (wday == 0) {
    if (day_of_month + 1 == last_day_of_month(month, year)) {
      /* If it's the last day of the month, go to the previous Friday */
      t -= 2 * DAY_SECONDS;
    } else {
      t += DAY_SECONDS; /* go to the next Monday */
    }
  }
  /* If it's a Saturday */
  else if (wday == 6) {
    if (day_of_month == 0) {
      /* If it's the first day of the month, go to the next Monday */
      t += 2 * DAY_SECONDS;
    } else {
      t -= DAY_SECONDS; /* go to the previous Friday */
    }
  }

  /* If it's a weekday */
  return cron_time(&t, &cal)->tm_mday;
}

/**
 * Reset the calendar setting all the fields provided to zero.
 */
static int
reset_max (struct tm* calendar, int field) {
  time_t res = 0;
  if (!calendar || -1 == field) {
    return 1;
  }
  switch (field) {
    case CRON_CF_SECOND: calendar->tm_sec = CRON_MAX_SECONDS - 1; break;
    case CRON_CF_MINUTE: calendar->tm_min = CRON_MAX_MINUTES - 1; break;
    case CRON_CF_HOUR_OF_DAY: calendar->tm_hour = CRON_MAX_HOURS - 1; break;
    case CRON_CF_DAY_OF_WEEK:
      calendar->tm_wday = CRON_MAX_DAYS_OF_WEEK - 1;
      break;
    case CRON_CF_DAY_OF_MONTH:
      calendar->tm_mday = last_day_of_month(calendar->tm_mon, calendar->tm_year);
      break;
    case CRON_CF_MONTH: calendar->tm_mon = CRON_MAX_MONTHS - 1; break;
    case CRON_CF_YEAR:
      /* I don't think this is supposed to happen ... */
      fprintf(stderr, "reset CRON_CF_YEAR\n");
      break;
    default:
      return 1; /* unknown field */
      ;
      ;
  }
  res = cron_mktime(calendar);
  return CRON_INVALID_INSTANT == res ? 1 : 0;
}

static int
reset_all (int (*fn)(struct tm* calendar, int field), struct tm* calendar, uint8_t* fields) {
  int i;
  int res = 0;
  if (!calendar || !fields) {
    return 1;
  }
  for (i = 0; i < CRON_CF_ARR_LEN; i++) {
    if (cron_get_bit(fields, i)) {
      res = fn(calendar, i);
      if (0 != res) {
        return res;
      }
    }
  }
  return 0;
}

#define reset_all_min(calendar, fields) reset_all(reset_min, calendar, fields);
#define reset_all_max(calendar, fields) reset_all(reset_max, calendar, fields);

static int
set_field (struct tm* calendar, int field, int val) {
  time_t res = 0;
  if (!calendar || -1 == field) {
    return 1;
  }
  switch (field) {
    case CRON_CF_SECOND: calendar->tm_sec = val; break;
    case CRON_CF_MINUTE: calendar->tm_min = val; break;
    case CRON_CF_HOUR_OF_DAY: calendar->tm_hour = val; break;
    case CRON_CF_DAY_OF_WEEK: calendar->tm_wday = val; break;
    case CRON_CF_DAY_OF_MONTH: calendar->tm_mday = val; break;
    case CRON_CF_MONTH: calendar->tm_mon = val; break;
    case CRON_CF_YEAR: calendar->tm_year = val; break;
    default: return 1; /* unknown field */
  }
  res = cron_mktime(calendar);
  return CRON_INVALID_INSTANT == res ? 1 : 0;
}

/**
 * Search the bits provided for the next set bit after the value provided,
 * and reset the calendar.
 */
static int
find_next_offset (uint8_t* bits, int max, int value, int offset, struct tm* calendar, int field, int nextField, uint8_t* lower_orders, int* res_out) {
  int notfound   = 0;
  int err        = 0;
  int next_value = next_set_bit(bits, max, value + offset, &notfound) - offset;
  /* roll over if needed */
  if (notfound) {
    err = add_to_field(calendar, nextField, 1);
    if (err) {
      goto return_error;
    }
    err = reset_min(calendar, field);
    if (err) {
      goto return_error;
    }
    notfound   = 0;
    next_value = next_set_bit(bits, max, 0, &notfound);
  }
  if (notfound || next_value != value) {
    err = reset_all_min(calendar, lower_orders);
    if (err) {
      goto return_error;
    }
    err = set_field(calendar, field, next_value);
    if (err) {
      goto return_error;
    }
  }
  return next_value;

return_error:
  *res_out = 1;
  return 0;
}

static int
find_next (uint8_t* bits, int max, int value, struct tm* calendar, int field, int nextField, uint8_t* lower_orders, int* res_out) {
  return find_next_offset(bits, max, value, 0, calendar, field, nextField, lower_orders, res_out);
}

static int
find_nextprev_day_condition (struct tm* calendar, uint8_t* days_of_month, int8_t* day_in_month, int day_of_month, uint8_t* days_of_week, int day_of_week, uint8_t* flags, int* changed) {
  static int day;
  if (*changed) {
    if (*flags) {
      if (cron_get_bit(flags, 0)) {
        day = last_day_of_month(calendar->tm_mon, calendar->tm_year);
      } else if (cron_get_bit(flags, 1)) {
        day = last_weekday_of_month(calendar->tm_mon, calendar->tm_year);
      } else if (cron_get_bit(flags, 2)) {
        day = closest_weekday(*day_in_month - 1, calendar->tm_mon, calendar->tm_year);
      }
    } else {
      if (*day_in_month < 0) {
        day = last_day_of_month(calendar->tm_mon, calendar->tm_year);
      }
    }
    *changed = 0;
  }

  if (!cron_get_bit(days_of_month, day_of_month)) {
    return 1;
  }
  if (!cron_get_bit(days_of_week, day_of_week)) {
    return 1;
  }
  if (*flags) {
    if ((cron_get_bit(flags, 0) && day != day_of_month - 1 - *day_in_month)) {
      return 1;
    }
    if ((cron_get_bit(flags, 1) && day != day_of_month - 1 - *day_in_month)) {
      return 1;
    }
    if ((cron_get_bit(flags, 2) && day != day_of_month)) {
      return 1;
    }
  } else {
    if (*day_in_month < 0 && (day_of_month < day + WEEK_DAYS * *day_in_month + 1 || day_of_month >= day + WEEK_DAYS * (*day_in_month + 1) + 1)) {
      return 1;
    }
    if (*day_in_month > 0 && (day_of_month < WEEK_DAYS * (*day_in_month - 1) + 1 || day_of_month >= WEEK_DAYS * *day_in_month + 1)) {
      return 1;
    }
  }
  return 0;
}

static int
find_nextprev_day (struct tm* calendar, uint8_t* days_of_month, int8_t* day_in_month, int day_of_month, uint8_t* days_of_week, int day_of_week, uint8_t* flags, uint8_t* resets, int* res_out, int offset) {
  int          err;
  unsigned int count   = 0;
  unsigned int max     = 366;
  int          changed = 1;
  int          year    = calendar->tm_year;
  int          month   = calendar->tm_mon;
  while (find_nextprev_day_condition(calendar, days_of_month, day_in_month, day_of_month, days_of_week, day_of_week, flags, &changed) && count++ < max) {
    err = add_to_field(calendar, CRON_CF_DAY_OF_MONTH, offset);

    if (err) {
      goto return_error;
    }
    day_of_month = calendar->tm_mday;
    day_of_week  = calendar->tm_wday;
    if (year != calendar->tm_year) {
      year    = calendar->tm_year;
      /* This should not be needed unless there is as single day month in libc.
       */
      changed = 1;
    }
    if (month != calendar->tm_mon) {
      month   = calendar->tm_mon;
      changed = 1;
    }
    if (offset > 0) {
      reset_all_min(calendar, resets);
    } else {
      reset_all_max(calendar, resets);
    }
  }
  return day_of_month;

return_error:
  *res_out = 1;
  return 0;
}

static int
find_next_day (struct tm* calendar, uint8_t* days_of_month, int8_t* day_in_month, int day_of_month, uint8_t* days_of_week, int day_of_week, uint8_t* flags, uint8_t* resets, int* res_out) {
  return find_nextprev_day(calendar, days_of_month, day_in_month, day_of_month, days_of_week, day_of_week, flags, resets, res_out, 1);
}

static int
do_nextprev (int (*find)(uint8_t* bits, int max, int value, struct tm* calendar, int field, int nextField, uint8_t* lower_orders, int* res_out), int (*find_day)(struct tm* calendar, uint8_t* days_of_month, int8_t* day_in_month, int day_of_month, uint8_t* days_of_week, int day_of_week, uint8_t* flags, uint8_t* resets, int* res_out), int (*find_offset)(uint8_t* bits, int max, int value, int offset, struct tm* calendar, int field, int nextField, uint8_t* lower_orders, int* res_out), cron_expr* expr, struct tm* calendar, int dot) {
  int     res = 0;
  uint8_t resets[1];
  uint8_t empty_list[1] = {0};
  int     value         = 0;
  int     update_value  = 0;

  for (;;) {
    *resets = 0;

    value   = calendar->tm_sec;
    update_value = find(expr->seconds, CRON_MAX_SECONDS + CRON_MAX_LEAP_SECONDS, value, calendar, CRON_CF_SECOND, CRON_CF_MINUTE, empty_list, &res);
    if (0 != res) {
      break;
    }
    if (value == update_value) {
      cron_set_bit(resets, CRON_CF_SECOND);
    } else if (update_value >= CRON_MAX_SECONDS) {
      continue;
    }

    value = calendar->tm_min;
    update_value = find(expr->minutes, CRON_MAX_MINUTES, value, calendar, CRON_CF_MINUTE, CRON_CF_HOUR_OF_DAY, resets, &res);
    if (0 != res) {
      break;
    }
    if (value == update_value) {
      cron_set_bit(resets, CRON_CF_MINUTE);
    } else {
      continue;
    }

    value = calendar->tm_hour;
    update_value = find(expr->hours, CRON_MAX_HOURS, value, calendar, CRON_CF_HOUR_OF_DAY, CRON_CF_DAY_OF_WEEK, resets, &res);
    if (0 != res) {
      break;
    }
    if (value == update_value) {
      cron_set_bit(resets, CRON_CF_HOUR_OF_DAY);
    } else {
      continue;
    }

    value        = calendar->tm_mday;
    update_value = find_day(
      calendar,
      expr->days_of_month,
      expr->day_in_month,
      value,
      expr->days_of_week,
      calendar->tm_wday,
      expr->flags,
      resets,
      &res
    );
    if (0 != res) {
      break;
    }
    if (value == update_value) {
      cron_set_bit(resets, CRON_CF_DAY_OF_MONTH);
    } else {
      continue;
    }

    value = calendar->tm_mon; /*day already adds one if no day in same value is found*/
    update_value = find(expr->months, CRON_MAX_MONTHS, value, calendar, CRON_CF_MONTH, CRON_CF_YEAR, resets, &res);
    if (0 != res) {
      break;
    }
    if (value != update_value) {
      if (abs(calendar->tm_year - dot) > CRON_MAX_YEARS_DIFF) {
        res = -1;
        break;
      }
      continue;
    }

    value = calendar->tm_year;
    update_value = find_offset(expr->years, CRON_MAX_YEARS - CRON_MIN_YEARS, value, YEAR_OFFSET - CRON_MIN_YEARS, calendar, CRON_CF_YEAR, CRON_CF_NEXT, resets, &res);
    if (0 != res) {
      break;
    }
    if (value == update_value) {
      break;
    }
  }

  return res;
}

static int
do_next (cron_expr* expr, struct tm* calendar, int dot) {
  return do_nextprev(find_next, find_next_day, find_next_offset, expr, calendar, dot);
}

static int
to_upper (char* str) {
  int i = 0;
  if (!str) {
    return 1;
  }
  for (i = 0; '\0' != str[i]; i++) {
    int c  = (int)str[i];
    str[i] = (char)toupper(c);
  }
  return 0;
}

static void
strreverse (char* begin, char* end) {
  char aux;
  while (end > begin) {
    aux = *end, *end-- = *begin, *begin++ = aux;
  }
}

/* included from
 * https://github.com/client9/stringencoders/blob/master/src/modp_numtoa.c */
size_t
to_string (size_t value, char* str) {
  char* wstr = str;
  /* Conversion. Number is reversed. */
  do {
    *wstr++ = (char)(48 + (value % 10));
  } while (value /= 10);
  *wstr = '\0';
  /* Reverse string */
  strreverse(str, wstr - 1);
  return (size_t)(wstr - str);
}

static char*
str_replace (char* orig, const char* rep, const char* with) {
  char*  result;    /* the return string */
  char*  ins;       /* the next insert point */
  char*  tmp;       /* varies */
  size_t len_rep;   /* length of rep */
  size_t len_with;  /* length of with */
  size_t len_front; /* distance between rep and end of last rep */
  size_t count;     /* number of replacements */
  if (!orig) {
    return NULL;
  }
  if (!rep) {
    rep = "";
  }
  if (!with) {
    with = "";
  }
  len_rep  = strlen(rep);
  len_with = strlen(with);

  ins      = orig;
  for (count = 0; NULL != (tmp = strstr(ins, rep)); ++count) {
    ins = tmp + len_rep;
  }

  /* first time through the loop, all the variable are set correctly
   from here on,
   tmp points to the end of the result string
   ins points to the next occurrence of rep in orig
   orig points to the remainder of orig after "end of rep"
   */
  tmp = result = (char*)cron_malloc(strlen(orig) + (len_with - len_rep) * count + 1);
  if (!result) {
    return NULL;
  }

  while (count--) {
    ins        = strstr(orig, rep);
    len_front  = (size_t)(ins - orig);
    tmp        = strncpy(tmp, orig, len_front) + len_front;
    tmp        = strcpy(tmp, with) + len_with;
    orig      += len_front + len_rep; /* move to next "end of rep" */
  }
  strcpy(tmp, orig);
  return result;
}

static int
parse_uint (const char* str, int* errcode) {
  char*    endptr = NULL;
  long int l;
  errno = 0;
  l     = strtol(str, &endptr, 10);
  if (errno == ERANGE || *endptr != '\0' || l < 0 || l > INT_MAX) {
    *errcode = 1;
    return 0;
  } else {
    *errcode = 0;
    return (int)l;
  }
}

static int
parse_int (const char* str, int* errcode) {
  return *str == '-' ? -parse_uint(str + 1, errcode) : parse_uint(str, errcode);
}

static char**
split_str (const char* str, char del, size_t* len_out) {
  size_t i     = 0;
  size_t stlen = 0;
  size_t len   = 0;
  int    accum = 0;
  char*  buf   = NULL;
  char** res   = NULL;
  size_t bi    = 0;
  size_t ri    = 0;
  char*  tmp   = NULL;

  if (!str) {
    goto return_error;
  }
  for (i = 0; '\0' != str[i]; i++) {
    stlen += 1;
    if (stlen >= CRON_MAX_STR_LEN_TO_SPLIT) {
      goto return_error;
    }
  }

  for (i = 0; i < stlen; i++) {
    int c = (unsigned char)str[i];
    if (del == str[i]) {
      if (accum > 0) {
        len   += 1;
        accum  = 0;
      }
    } else if (!isspace(c)) {
      accum += 1;
    }
  }
  /* tail */
  if (accum > 0) {
    len += 1;
  }
  if (0 == len) {
    return NULL;
  }

  buf = (char*)cron_malloc(stlen + 1);
  if (!buf) {
    goto return_error;
  }
  memset(buf, 0, stlen + 1);
  res = (char**)cron_malloc(len * sizeof(char*));
  if (!res) {
    goto return_error;
  }
  memset(res, 0, len * sizeof(char*));

  for (i = 0; i < stlen; i++) {
    int c = (unsigned char)str[i];
    if (del == str[i]) {
      if (bi > 0) {
        if (ri >= len) {
          goto return_error;
        }
        tmp = strdupl(buf, bi);
        if (!tmp) {
          goto return_error;
        }
        res[ri++] = tmp;
        memset(buf, 0, stlen + 1);
        bi = 0;
      }
    } else if (!isspace(c)) {
      buf[bi++] = str[i];
    }
  }
  /* tail */
  if (bi > 0) {
    if (ri >= len) {
      goto return_error;
    }
    tmp = strdupl(buf, bi);
    if (!tmp) {
      goto return_error;
    }
    res[ri++] = tmp;
  }
  cron_free(buf);
  *len_out = len;
  return res;

return_error:
  if (buf) {
    cron_free(buf);
  }
  free_splitted(res, len);
  *len_out = 0;
  return NULL;
}

static char*
replace_ordinals (char* value, const char* const* arr, size_t arr_len) {
  size_t i;
  char*  cur   = value;
  char*  res   = NULL;
  int    first = 1;
  char   strnum[CRON_SIZE_STRING_MAX_LEN + 1];

  for (i = 0; i < arr_len; i++) {
    to_string(i, strnum);

    res = str_replace(cur, arr[i], strnum);
    if (!first) {
      cron_free(cur);
    }
    if (!res) {
      return NULL;
    }
    cur = res;
    if (first) {
      first = 0;
    }
  }
  return res;
}

static size_t
has_char (const char* str, char ch) {
  size_t i = 0;
  if (!str) {
    return 0;
  }
  for (i = 0; str[i] != '\0'; i++) {
    /* Match at first character is now treated as failure for convenience! */
    if (str[i] == ch) {
      return i;
    }
  }
  return 0;
}

static void
get_range (const char* field, int min, int max, int* res, const char** error) {
  char** parts = NULL;
  size_t len   = 0;
  int    err   = 0;
  int    val;
  if (!res) {
    *error = "NULL";
    goto return_error;
  }

  res[0] = 0;
  res[1] = 0;
  if (1 == strlen(field) && '*' == field[0]) {
    res[0] = min;
    res[1] = max - 1;
  } else if (!has_char(field, '-')) {
    err = 0;
    val = parse_uint(field, &err);
    if (err) {
      *error = "Unsigned integer parse error 1";
      goto return_error;
    }

    res[0] = val;
    res[1] = val;
  } else {
    parts = split_str(field, '-', &len);
    if (2 != len) {
      *error = "Specified range requires two fields";
      goto return_error;
    }
    err    = 0;
    res[0] = parse_uint(parts[0], &err);
    if (err) {
      *error = "Unsigned integer parse error 2";
      goto return_error;
    }
    res[1] = parse_uint(parts[1], &err);
    if (err) {
      *error = "Unsigned integer parse error 3";
      goto return_error;
    }
  }
  if (res[0] >= max || res[1] >= max) {
    *error = "Specified range exceeds maximum";
    goto return_error;
  }
  if (res[0] < min || res[1] < min) {
    *error = "Specified range is less than minimum";
    goto return_error;
  }
  if (res[0] > res[1]) {
    *error = "Specified range start exceeds range end";
    goto return_error;
  }

  free_splitted(parts, len);
  *error = NULL;
  return;

return_error:
  free_splitted(parts, len);
}

static void
set_number_hits_offset (const char* value, uint8_t* target, int min, int max, int offset, const char** error) {
  size_t i   = 0;
  int    i1  = 0;
  size_t len = 0;
  int    range[2];
  int    err = 0;
  int    delta;

  char** fields = split_str(value, ',', &len);
  if (!fields) {
    *error = "Comma split error";
    goto return_result;
  }

  for (i = 0; i < len; i++) {
    if (!has_char(fields[i], '/')) {
      /* Not an incrementer so it must be a range (possibly empty) */

      get_range(fields[i], min, max, range, error);

      if (*error) {
        goto return_result;
      }

      for (i1 = range[0]; i1 <= range[1]; i1++) {
        cron_set_bit(target, i1 + offset);
      }
    } else {
      size_t len2  = 0;
      char** split = split_str(fields[i], '/', &len2);
      if (2 != len2) {
        *error = "Incrementer must have two fields";
        free_splitted(split, len2);
        goto return_result;
      }
      get_range(split[0], min, max, range, error);
      if (*error) {
        free_splitted(split, len2);
        goto return_result;
      }
      if (!has_char(split[0], '-')) {
        range[1] = max - 1;
      }
      delta = parse_uint(split[1], &err);
      if (err) {
        *error = "Unsigned integer parse error 4";
        free_splitted(split, len2);
        goto return_result;
      }
      if (0 == delta) {
        *error = "Incrementer may not be zero";
        free_splitted(split, len2);
        goto return_result;
      }
      for (i1 = range[0]; i1 <= range[1]; i1 += delta) {
        cron_set_bit(target, i1 + offset);
      }
      free_splitted(split, len2);
    }
  }
  goto return_result;

return_result:
  free_splitted(fields, len);
}

static void
set_number_hits (const char* value, uint8_t* target, int min, int max, const char** error) {
  set_number_hits_offset(value, target, min, max, 0, error);
}

static void
set_months (char* value, uint8_t* targ, const char** error) {
  int i;
  int max        = CRON_MAX_MONTHS;

  char* replaced = NULL;

  to_upper(value);
  replaced = replace_ordinals(value, MONTHS_ARR, CRON_MONTHS_ARR_LEN);
  if (!replaced) {
    *error = "Invalid month format";
    return;
  }
  set_number_hits(replaced, targ, 1, max + 1, error);
  cron_free(replaced);

  /* ... and then rotate it to the front of the months */
  for (i = 1; i <= max; i++) {
    if (cron_get_bit(targ, i)) {
      cron_set_bit(targ, i - 1);
      cron_del_bit(targ, i);
    }
  }
}

static void
set_years (char* value, uint8_t* targ, const char** error) {
  int min = CRON_MIN_YEARS;
  int max = CRON_MAX_YEARS;
  set_number_hits_offset(value, targ, min, max, -min, error);
}

static void
set_days_of_week (char* field, uint8_t* days_of_week, int8_t* day_in_month, const char** error) {
  const int max      = 7;
  char*     replaced = NULL;
  size_t    pos;
  int       err;

  to_upper(field);
  if (1 == strlen(field) && '?' == field[0]) {
    field[0] = '*';
  }
  if (field[0] == 'L' && 1 == strlen(field)) {
    field[0] = '0';
  } else if ((pos = has_char(field, 'L')) != 0) {
    if (pos + 1 != strlen(field)) {
      *error = "L has to be last";
      return;
    }
    field[pos]    = '\0';
    *day_in_month = -1;
  } else if ((pos = has_char(field, '#')) != 0) {
    if (pos + 1 == strlen(field)) {
      *error = "# can't be last";
      return;
    }
    field[pos]    = '\0';
    *day_in_month = (int8_t)parse_int(field + pos + 1, &err);
    if (err) {
      *error = "Unsigned integer parse error 5";
      return;
    }
    if (*day_in_month > 5 || *day_in_month < -5) {
      *error = "# can follow with -5..5";
      return;
    }
  }
  replaced = replace_ordinals(field, DAYS_ARR, CRON_DAYS_ARR_LEN);
  if (!replaced) {
    *error = "Invalid day format";
    return;
  }
  set_number_hits(replaced, days_of_week, 0, max + 1, error);
  cron_free(replaced);

  if (cron_get_bit(days_of_week, 7)) {
    /* Sunday can be represented as 0 or 7*/
    cron_set_bit(days_of_week, 0);
    cron_del_bit(days_of_week, 7);
  }
  return;
}

static int
set_days_of_month (char* field, uint8_t* days_of_month, uint8_t* days_of_week, int8_t* day_in_month, uint8_t* flags, const char** error) {
  int    i1;
  int    ret = 0;
  size_t pos;
  int    err;

  /* Days of month start with 1 (in Cron and Calendar) so add one */
  if (1 == strlen(field) && '?' == field[0]) {
    field[0] = '*';
  }
  to_upper(field);
  if (field[0] == 'L') {
    *day_in_month = -1;
    ret           = 1;
    if (1 == strlen(field) || field[1] == '-') {
      /* Note 0..6 and not 1..7, see end of set_days_of_week. */
      for (i1 = 0; i1 <= 6; i1++) {
        cron_set_bit(days_of_week, i1);
      }
      cron_set_bit(flags, 0);
      field[0] = '*';
      if (field[1] == '-') {
        *day_in_month += (int8_t)parse_int(field + 1, &err);
        if (err) {
          *error = "Unsigned integer parse error 6";
          return ret;
        }
        field[1] = '\0';
      }
    } else if (field[1] == 'W' && 2 == strlen(field)) {
      for (i1 = 1; i1 <= 5; i1++) {
        cron_set_bit(days_of_week, i1);
      }
      cron_set_bit(flags, 1);
      field[0] = '*';
      field[1] = '\0';
    } else {
      *error = "L should continue with W or -value";
    }
  } else if (field[0] == 'W' && 1 == strlen(field)) {
    ret = 1;
    for (i1 = 1; i1 <= 5; i1++) {
      cron_set_bit(days_of_week, i1);
    }
    field[0] = '*';
  } else if ((pos = has_char(field, 'W')) != 0) {
    if (pos + 1 != strlen(field)) {
      *error = "W has to be last";
      return ret;
    }
    field[pos]    = '\0';
    *day_in_month = (int8_t)parse_uint(field, &err);
    if (err) {
      *error = "Unsigned integer parse error 7";
      return ret;
    }
    for (i1 = 1; i1 <= 5; i1++) {
      cron_set_bit(days_of_week, i1);
    }
    cron_set_bit(flags, 2);
    field[0] = '*';
    field[1] = '\0';
  } else {
    *day_in_month = 0;
  }
  set_number_hits(field, days_of_month, 1, CRON_MAX_DAYS_OF_MONTH, error);
  return ret;
}

void
cron_parse_expr (const char* expression, cron_expr* target, const char** error) {
  const char* err_local;
  size_t      len    = 0;
  char**      fields = NULL;
  char*       field;
  int         ret;
  int         pos = 0;
  if (!error) {
    error = &err_local;
  }
  *error = NULL;
  if (!expression) {
    *error = "Invalid NULL expression";
    goto return_res;
  }
  if (!target) {
    *error = "Invalid NULL target";
    goto return_res;
  }

  if ('@' == expression[0]) {
    expression++;
    if (!strcmp("annually", expression) || !strcmp("yearly", expression)) {
      expression = "0 0 0 1 1 * *";
    } else if (!strcmp("monthly", expression)) {
      expression = "0 0 0 1 * * *";
    } else if (!strcmp("weekly", expression)) {
      expression = "0 0 0 * * 0 *";
    } else if (!strcmp("daily", expression) || !strcmp("midnight", expression)) {
      expression = "0 0 0 * * * *";
    } else if (!strcmp("hourly", expression)) {
      expression = "0 0 * * * * *";
    } else if (!strcmp("minutely", expression)) {
      expression = "0 * * * * * *";
    } else if (!strcmp("secondly", expression)) {
      expression = "* * * * * * *";
    } else if (!strcmp("reboot", expression)) {
      *error = "@reboot not implemented";
      goto return_res;
    }
  }

  fields = split_str(expression, ' ', &len);
  if (len < 5 || len > 7) {
    *error = "Invalid number of fields, expression must consist of 5-7 fields";
    goto return_res;
  }
  memset(target, 0, sizeof(*target));
  if (len > 5) {
    if (fields[pos][0] == 'L') {
      field = fields[pos++];
      set_number_hits(field + 1, target->seconds, 0, CRON_MAX_SECONDS + CRON_MAX_LEAP_SECONDS, error);
    } else {
      set_number_hits(fields[pos++], target->seconds, 0, CRON_MAX_SECONDS, error);
    }
    if (*error) {
      goto return_res;
    }
  } else {
    set_number_hits("0", target->seconds, 0, CRON_MAX_SECONDS, error);
  }
  set_number_hits(fields[pos++], target->minutes, 0, CRON_MAX_MINUTES, error);
  if (*error) {
    goto return_res;
  }
  set_number_hits(fields[pos++], target->hours, 0, CRON_MAX_HOURS, error);
  if (*error) {
    goto return_res;
  }
  ret = set_days_of_month(
    fields[pos++],
    target->days_of_month,
    target->days_of_week,
    target->day_in_month,
    target->flags,
    error
  );
  if (*error) {
    goto return_res;
  }
  set_months(fields[pos++], target->months, error);
  if (*error) {
    goto return_res;
  }
  if (0 == ret) {
    set_days_of_week(fields[pos++], target->days_of_week, target->day_in_month, error);
    if (*error) {
      goto return_res;
    }
  }
  if (len < 7) {
    set_years((char*)"*", target->years, error);
  } else {
    set_years(fields[pos], target->years, error);
  }
  if (*error) {
    goto return_res;
  }
  goto return_res;

return_res:
  free_splitted(fields, len);
}

time_t
cron_nextprev (int (*fn)(cron_expr* expr, struct tm* calendar, int dot), cron_expr* expr, time_t date, int offset) {
  /*
   The plan:

   1 Round up to the next whole second

   2 If seconds match move on, otherwise find the next match:
   2.1 If next match is in the next minute then roll forwards

   3 If minute matches move on, otherwise find the next match
   3.1 If next match is in the next hour then roll forwards
   3.2 Reset the seconds and go to 2

   4 If hour matches move on, otherwise find the next match
   4.1 If next match is in the next day then roll forwards,
   4.2 Reset the minutes and seconds and go to 2

   ...
   */
  struct tm  calval;
  struct tm* calendar;
  int        res;
  time_t     original;
  time_t     calculated;
  if (!expr) {
    return CRON_INVALID_INSTANT;
  }
  memset(&calval, 0, sizeof(struct tm));
  calendar = cron_time(&date, &calval);
  if (!calendar) {
    return CRON_INVALID_INSTANT;
  }
  original = cron_mktime(calendar);
  if (CRON_INVALID_INSTANT == original) {
    return CRON_INVALID_INSTANT;
  }

  res = fn(expr, calendar, calendar->tm_year);
  if (0 != res) {
    return CRON_INVALID_INSTANT;
  }

  calculated = cron_mktime(calendar);
  if (CRON_INVALID_INSTANT == calculated) {
    return CRON_INVALID_INSTANT;
  }
  if (calculated == original) {
    /* We arrived at the original timestamp - round up to the next whole second
     * and try again... */
    res = add_to_field(calendar, CRON_CF_SECOND, offset);
    if (0 != res) {
      return CRON_INVALID_INSTANT;
    }
    res = fn(expr, calendar, calendar->tm_year);
    if (0 != res) {
      return CRON_INVALID_INSTANT;
    }
  }

  return cron_mktime(calendar);
}

time_t
cron_next (cron_expr* expr, time_t date) {
  return cron_nextprev(do_next, expr, date, 1);
}

/* https://github.com/staticlibs/ccronexpr/pull/8 */

static int
prev_set_bit (uint8_t* bits, int from_index, int to_index, int* notfound) {
  int i;
  if (!bits) {
    *notfound = 1;
    return 0;
  }
  for (i = from_index; i >= to_index; i--) {
    if (cron_get_bit(bits, i)) {
      return i;
    }
  }
  *notfound = 1;
  return 0;
}

/**
 * Search the bits provided for the next set bit after the value provided,
 * and reset the calendar.
 */
static int
find_prev_offset (uint8_t* bits, int max, int value, int offset, struct tm* calendar, int field, int nextField, uint8_t* lower_orders, int* res_out) {
  int notfound   = 0;
  int err        = 0;
  int next_value = prev_set_bit(bits, value, 0, &notfound);
  offset         = offset;
  /* roll under if needed */
  if (notfound) {
    err = add_to_field(calendar, nextField, -1);
    if (err) {
      goto return_error;
    }
    err = reset_max(calendar, field);
    if (err) {
      goto return_error;
    }
    notfound   = 0;
    next_value = prev_set_bit(bits, max - 1, value, &notfound);
  }
  if (notfound || next_value != value) {
    err = reset_all_max(calendar, lower_orders);
    if (err) {
      goto return_error;
    }
    err = set_field(calendar, field, next_value);
    if (err) {
      goto return_error;
    }
  }
  return next_value;

return_error:
  *res_out = 1;
  return 0;
}

static int
find_prev (uint8_t* bits, int max, int value, struct tm* calendar, int field, int nextField, uint8_t* lower_orders, int* res_out) {
  return find_prev_offset(bits, max, value, 0, calendar, field, nextField, lower_orders, res_out);
}

static int
find_prev_day (struct tm* calendar, uint8_t* days_of_month, int8_t* day_in_month, int day_of_month, uint8_t* days_of_week, int day_of_week, uint8_t* flags, uint8_t* resets, int* res_out) {
  return find_nextprev_day(calendar, days_of_month, day_in_month, day_of_month, days_of_week, day_of_week, flags, resets, res_out, -1);
}

static int
do_prev (cron_expr* expr, struct tm* calendar, int dot) {
  return do_nextprev(find_prev, find_prev_day, find_prev_offset, expr, calendar, dot);
}

time_t
cron_prev (cron_expr* expr, time_t date) {
  return cron_nextprev(do_prev, expr, date, -1);
}
