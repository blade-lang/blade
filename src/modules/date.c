#include "date.h"
#include "btime.h"

#ifdef _WIN32
#include "win32.h"
#endif

#include <time.h>

#define ADD_TIME(n, l, v)                                                      \
  dict_add_entry(vm, dict, STRING_L_VAL(n, l), NUMBER_VAL(v))

#define ADD_B_TIME(n, l, v)                                                     \
  dict_add_entry(vm, dict, STRING_L_VAL(n, l), BOOL_VAL(v))

#define ADD_S_TIME(n, l, v, g)                                                  \
  dict_add_entry(vm, dict, STRING_L_VAL(n, l), STRING_L_VAL(v, g))

DECLARE_MODULE_METHOD(date____mktime) {
  ENFORCE_ARG_RANGE(mktime, 1, 8);

  if (arg_count < 7) {
    for (int i = 0; i < arg_count; i++) {
      ENFORCE_ARG_TYPE(mktime, i, IS_NUMBER);
    }
  } else {
    for (int i = 0; i < 6; i++) {
      ENFORCE_ARG_TYPE(mktime, i, IS_NUMBER);
    }
    ENFORCE_ARG_TYPE(mktime, 6, IS_BOOL);
  }

  int year = -1900, month = 1, day = 1, hour = 0, minute = 0, seconds = 0,
      is_dst = 0;
  year += AS_NUMBER(args[0]);

  if (arg_count > 1)
    month = AS_NUMBER(args[1]);
  if (arg_count > 2)
    day = AS_NUMBER(args[2]);
  if (arg_count > 3)
    hour = AS_NUMBER(args[3]);
  if (arg_count > 4)
    minute = AS_NUMBER(args[4]);
  if (arg_count > 5)
    seconds = AS_NUMBER(args[5]);
  if (arg_count > 6)
    is_dst = AS_BOOL(args[5]) ? 1 : 0;

  struct tm t;
  t.tm_year = year;
  t.tm_mon = month - 1;
  t.tm_mday = day;
  t.tm_hour = hour;
  t.tm_min = minute;
  t.tm_sec = seconds;
  t.tm_isdst = is_dst;

  RETURN_NUMBER((long) mktime(&t));
}

DECLARE_MODULE_METHOD(date__localtime) {
  struct timeval raw_time;
  gettimeofday(&raw_time, NULL);
  struct tm *time_info = localtime(&raw_time.tv_sec);

  b_obj_dict *dict = new_dict(vm);
  push(vm, OBJ_VAL(dict));

  ADD_TIME("year", 4, (double) time_info->tm_year + 1900);
  ADD_TIME("month", 5, (double) time_info->tm_mon + 1);
  ADD_TIME("day", 3, time_info->tm_mday);
  ADD_TIME("week_day", 8, time_info->tm_wday);
  ADD_TIME("year_day", 8, time_info->tm_yday);
  ADD_TIME("hour", 4, time_info->tm_hour);
  ADD_TIME("minute", 6, time_info->tm_min);
  if (time_info->tm_sec <= 59) {
    ADD_TIME("seconds", 7, time_info->tm_sec);
  } else {
    ADD_TIME("seconds", 7, 59);
  }
  ADD_TIME("microseconds", 12, (double) raw_time.tv_usec);

  ADD_B_TIME("is_dst", 6, time_info->tm_isdst == 1 ? true : false);

#ifndef _WIN32
  // set time zone
  ADD_S_TIME("zone", 4, time_info->tm_zone, (int)strlen(time_info->tm_zone));
  // setting gmt offset
  ADD_TIME("gmt_offset", 10, time_info->tm_gmtoff);
#else
  // set time zone
  ADD_S_TIME("zone", 4, "", 0);
  // setting gmt offset
  ADD_TIME("gmt_offset", 10, 0);
#endif

  pop(vm);
  RETURN_OBJ(dict);
}

DECLARE_MODULE_METHOD(date__gmtime) {
  struct timeval raw_time;
  gettimeofday(&raw_time, NULL);
  struct tm *time_info = gmtime(&raw_time.tv_sec);

  b_obj_dict *dict = new_dict(vm);
  push(vm, OBJ_VAL(dict));

  ADD_TIME("year", 4, (double) time_info->tm_year + 1900);
  ADD_TIME("month", 5, (double) time_info->tm_mon + 1);
  ADD_TIME("day", 3, time_info->tm_mday);
  ADD_TIME("week_day", 8, time_info->tm_wday);
  ADD_TIME("year_day", 8, time_info->tm_yday);
  ADD_TIME("hour", 4, time_info->tm_hour);
  ADD_TIME("minute", 6, time_info->tm_min);
  if (time_info->tm_sec <= 59) {
    ADD_TIME("seconds", 7, time_info->tm_sec);
  } else {
    ADD_TIME("seconds", 7, 59);
  }
  ADD_TIME("microseconds", 12, (double) raw_time.tv_usec);

  ADD_B_TIME("is_dst", 6, time_info->tm_isdst == 1 ? true : false);

#ifndef _WIN32
  // set time zone
  ADD_S_TIME("zone", 4, time_info->tm_zone, (int)strlen(time_info->tm_zone));
  // setting gmt offset
  ADD_TIME("gmt_offset", 10, time_info->tm_gmtoff);
#else
  // set time zone
  ADD_S_TIME("zone", 4, "", 0);
  // setting gmt offset
  ADD_TIME("gmt_offset", 10, 0);
#endif

  pop(vm);
  RETURN_OBJ(dict);
}

CREATE_MODULE_LOADER(date) {
  static b_func_reg date_class_functions[] = {
      {"localtime", true,  GET_MODULE_METHOD(date__localtime)},
      {"gmtime",    true,  GET_MODULE_METHOD(date__gmtime)},
      {"__mktime",  false, GET_MODULE_METHOD(date____mktime)},
      {NULL,        false, NULL},
  };

  static b_class_reg klasses[] = {
      {"Date", NULL, date_class_functions},
      {NULL,   NULL, NULL},
  };

  static b_module_reg module = {NULL, klasses};

  return module;
}

#undef ADD_TIME
#undef ADD_B_TIME
#undef ADD_S_TIME