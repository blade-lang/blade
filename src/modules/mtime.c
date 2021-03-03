#include "mtime.h"

#include <time.h>

#define ADD_TIME(n, l, v)                                                      \
  dict_add_entry(vm, dict, OBJ_VAL(copy_string(vm, n, l)), NUMBER_VAL(v))

DECLARE_MODULE_METHOD(time__localtime) {
  time_t rawtime;
  time(&rawtime);
  struct tm *timeinfo = localtime(&rawtime);

  b_obj_dict *dict = new_dict(vm);

  ADD_TIME("year", 4, (double)timeinfo->tm_year + 1900);
  ADD_TIME("month", 5, (double)timeinfo->tm_mon + 1);
  ADD_TIME("day", 3, timeinfo->tm_mday);
  ADD_TIME("hour", 4, timeinfo->tm_hour);
  ADD_TIME("minute", 6, timeinfo->tm_min);
  if (timeinfo->tm_sec <= 59) {
    ADD_TIME("second", 6, timeinfo->tm_sec);
  } else {
    ADD_TIME("second", 6, 59);
  }

  RETURN_OBJ(dict);
}

DECLARE_MODULE_METHOD(time__gmtime) {
  time_t rawtime;
  time(&rawtime);
  struct tm *timeinfo = gmtime(&rawtime);

  b_obj_dict *dict = new_dict(vm);

  ADD_TIME("year", 4, (double)timeinfo->tm_year + 1900);
  ADD_TIME("month", 5, (double)timeinfo->tm_mon + 1);
  ADD_TIME("day", 3, timeinfo->tm_mday);
  ADD_TIME("hour", 4, timeinfo->tm_hour);
  ADD_TIME("minute", 6, timeinfo->tm_min);
  if (timeinfo->tm_sec <= 59) {
    ADD_TIME("second", 6, timeinfo->tm_sec);
  } else {
    ADD_TIME("second", 6, 59);
  }

  RETURN_OBJ(dict);
}

CREATE_MODULE_LOADER(time) {
  static b_func_reg date_class_functions[] = {
      {"localtime", true, GET_MODULE_METHOD(time__localtime)},
      {"gmtime", true, GET_MODULE_METHOD(time__gmtime)},
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