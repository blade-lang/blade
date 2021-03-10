#include "date.h"
#include "btime.h"

#include <time.h>

#define ADD_TIME(n, l, v)                                                      \
  dict_add_entry(vm, dict, OBJ_VAL(copy_string(vm, n, l)), NUMBER_VAL(v))

#define ADD_BTIME(n, l, v)                                                     \
  dict_add_entry(vm, dict, OBJ_VAL(copy_string(vm, n, l)), BOOL_VAL(v))

#define ADD_STIME(n, l, v, g)                                                  \
  dict_add_entry(vm, dict, OBJ_VAL(copy_string(vm, n, l)),                     \
                 OBJ_VAL(copy_string(vm, v, g)))

DECLARE_MODULE_METHOD(date__localtime) {
  struct timeval rawtime;
  gettimeofday(&rawtime, NULL);
  struct tm *timeinfo = localtime(&rawtime.tv_sec);

  b_obj_dict *dict = new_dict(vm);

  ADD_TIME("year", 4, (double)timeinfo->tm_year + 1900);
  ADD_TIME("month", 5, (double)timeinfo->tm_mon + 1);
  ADD_TIME("day", 3, timeinfo->tm_mday);
  ADD_TIME("week_day", 8, timeinfo->tm_wday);
  ADD_TIME("year_day", 8, timeinfo->tm_yday);
  ADD_TIME("hour", 4, timeinfo->tm_hour);
  ADD_TIME("minute", 6, timeinfo->tm_min);
  if (timeinfo->tm_sec <= 59) {
    ADD_TIME("seconds", 6, timeinfo->tm_sec);
  } else {
    ADD_TIME("seconds", 6, 59);
  }
  ADD_TIME("microseconds", 11, rawtime.tv_usec);

  ADD_BTIME("is_dst", 6, timeinfo->tm_isdst == 1 ? true : false);
  // set time zone
  ADD_STIME("zone", 4, timeinfo->tm_zone, (int)strlen(timeinfo->tm_zone));

  // setting gmt offset
  ADD_TIME("gmt_offset", 10, timeinfo->tm_gmtoff);

  RETURN_OBJ(dict);
}

DECLARE_MODULE_METHOD(date__gmtime) {
  struct timeval rawtime;
  gettimeofday(&rawtime, NULL);
  struct tm *timeinfo = gmtime(&rawtime.tv_sec);

  b_obj_dict *dict = new_dict(vm);

  ADD_TIME("year", 4, (double)timeinfo->tm_year + 1900);
  ADD_TIME("month", 5, (double)timeinfo->tm_mon + 1);
  ADD_TIME("day", 3, timeinfo->tm_mday);
  ADD_TIME("week_day", 8, timeinfo->tm_wday);
  ADD_TIME("year_day", 8, timeinfo->tm_yday);
  ADD_TIME("hour", 4, timeinfo->tm_hour);
  ADD_TIME("minute", 6, timeinfo->tm_min);
  if (timeinfo->tm_sec <= 59) {
    ADD_TIME("seconds", 6, timeinfo->tm_sec);
  } else {
    ADD_TIME("seconds", 6, 59);
  }
  ADD_TIME("microseconds", 11, rawtime.tv_usec);

  ADD_BTIME("is_dst", 6, timeinfo->tm_isdst == 1 ? true : false);
  // set time zone
  ADD_STIME("zone", 4, timeinfo->tm_zone, (int)strlen(timeinfo->tm_zone));

  // setting gmt offset
  ADD_TIME("gmt_offset", 10, timeinfo->tm_gmtoff);

  RETURN_OBJ(dict);
}

CREATE_MODULE_LOADER(time) {
  static b_func_reg date_class_functions[] = {
      {"localtime", true, GET_MODULE_METHOD(date__localtime)},
      {"gmtime", true, GET_MODULE_METHOD(date__gmtime)},
      {NULL, false, NULL},
  };

  static b_class_reg klasses[] = {
      {"Date", NULL, date_class_functions},
      {NULL, NULL, NULL},
  };

  static b_module_reg module = {NULL, klasses};

  return module;
}

#undef ADD_TIME
#undef ADD_BTIME
#undef ADD_STIME