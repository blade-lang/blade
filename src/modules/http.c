#include "http.h"

#include <curl/curl.h>

DECLARE_MODULE_METHOD(http___client) {
  ENFORCE_ARG_COUNT(__client, 7);

  ENFORCE_ARG_TYPE(__client, 0, IS_STRING);
  ENFORCE_ARG_TYPE(__client, 1, IS_STRING);
  ENFORCE_ARG_TYPE(__client, 2, IS_STRING);
  ENFORCE_ARG_TYPE(__client, 3, IS_NUMBER);
  ENFORCE_ARG_TYPE(__client, 4, IS_BOOL);
  ENFORCE_ARG_TYPE(__client, 5, IS_BOOL);
  ENFORCE_ARG_TYPE(__client, 6, IS_BOOL);

  b_obj_string *url = AS_STRING(args[0]);
  b_obj_string *user_agent = AS_STRING(args[1]);
  b_obj_string *referer = AS_STRING(args[2]);
  double timeout = AS_NUMBER(args[3]);
  bool follow_redirect = AS_BOOL(args[4]);
  bool skip_hostname_verification = AS_BOOL(args[5]);
  bool skip_peer_verification = AS_BOOL(args[6]);

  CURL *curl = curl_easy_init();

  if (curl) {
    curl_easy_setopt(curl, CURLOPT_URL, url->chars);

    if (user_agent->length > 0) {
      curl_easy_setopt(curl, CURLOPT_USERAGENT, user_agent->chars);
    }

    if (skip_hostname_verification) {
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    } else {
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 1L);
    }

    if (skip_peer_verification) {
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    } else {
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    }

    if (follow_redirect) {
      curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    } else {
      curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0L);
    }

    if (referer->length > 0) {
      curl_easy_setopt(curl, CURLOPT_AUTOREFERER, 1L);
      curl_easy_setopt(curl, CURLOPT_REFERER, referer->chars);
    } else {
      curl_easy_setopt(curl, CURLOPT_AUTOREFERER, 1L);
    }

    if (timeout != -1) {
      curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, (long)timeout);
    }

    // general options...
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
    curl_easy_setopt(curl, CURLOPT_HEADER, 1L);

    FILE *body_file = tmpfile();
    // for writing the body
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, body_file);

    CURLcode res_code = curl_easy_perform(curl);

    long status_code = 0;
    curl_easy_getinfo(curl, CURLINFO_HTTP_CODE, &status_code);

    b_value error = NIL_VAL;
    if (res_code != CURLE_OK) {
      const char *err = curl_easy_strerror(res_code);
      error = OBJ_VAL(copy_string(vm, err, (int)strlen(err)));
    }

    fseek(body_file, 0L, SEEK_END);
    size_t body_size = ftell(body_file);
    rewind(body_file);

    char *body_content = (char *)malloc(body_size + 1);
    fread(body_content, sizeof(char), body_size, body_file);

    fclose(body_file);

    curl_easy_cleanup(curl);

    b_obj_list *list = new_list(vm);
    write_list(vm, list, NUMBER_VAL(status_code));
    write_list(vm, list, error);
    write_list(vm, list,
               OBJ_VAL(copy_string(vm, body_content, (int)body_size)));

    RETURN_OBJ(list);
  }

  RETURN_ERROR("unable to initialize client");
}

CREATE_MODULE_LOADER(http) {

  static b_func_reg http_class_functions[] = {
      {"__client", false, GET_MODULE_METHOD(http___client)},
      {NULL, false, NULL},
  };

  static b_field_reg http_class_fields[] = {
      {NULL, false, NULL},
  };

  static b_class_reg classes[] = {
      {"HttpClient", http_class_fields, http_class_functions},
      {NULL, NULL, NULL},
  };

  static b_module_reg module = {NULL, classes};

  return module;
}