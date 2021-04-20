#include "http.h"

#include <curl/curl.h>
#include <string.h>

static inline size_t http_module_write_header(void *ptr, size_t size,
                                              size_t nmemb, void *stream) {
  int *data = (int *)stream;
  *data += (int)(nmemb * size);
  return nmemb * size;
}

DECLARE_MODULE_METHOD(http___client) {
  ENFORCE_ARG_COUNT(__client, 13);

  ENFORCE_ARG_TYPE(__client, 0, IS_STRING);
  ENFORCE_ARG_TYPE(__client, 1, IS_STRING);
  ENFORCE_ARG_TYPE(__client, 2, IS_STRING);
  ENFORCE_ARG_TYPE(__client, 3, IS_DICT);
  ENFORCE_ARG_TYPE(__client, 4, IS_NUMBER);
  ENFORCE_ARG_TYPE(__client, 5, IS_BOOL);
  ENFORCE_ARG_TYPE(__client, 6, IS_BOOL);
  ENFORCE_ARG_TYPE(__client, 7, IS_BOOL);
  //  ENFORCE_ARG_TYPE(__client, 8, IS_FILE);
  //  ENFORCE_ARG_TYPE(__client, 9, IS_FILE);
  ENFORCE_ARG_TYPE(__client, 10, IS_STRING);

  b_obj_string *url = AS_STRING(args[0]);
  b_obj_string *user_agent = AS_STRING(args[1]);
  b_obj_string *referer = AS_STRING(args[2]);
  b_obj_dict *headers = AS_DICT(args[3]);
  double timeout = AS_NUMBER(args[4]);
  bool follow_redirect = AS_BOOL(args[5]);
  bool skip_hostname_verification = AS_BOOL(args[6]);
  bool skip_peer_verification = AS_BOOL(args[7]);
  b_obj_string *request_type = AS_STRING(args[10]);
  bool no_expect = AS_BOOL(args[12]);

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

    if (!IS_NIL(args[8])) {
      ENFORCE_ARG_TYPE(__client, 8, IS_FILE);
      b_obj_file *file = AS_FILE(args[8]);
      curl_easy_setopt(curl, CURLOPT_CAPATH, realpath(file->path->chars, NULL));
    }

    if (!IS_NIL(args[9])) {
      ENFORCE_ARG_TYPE(__client, 9, IS_FILE);
      b_obj_file *file = AS_FILE(args[9]);
      curl_easy_setopt(curl, CURLOPT_COOKIEFILE,
                       realpath(file->path->chars, NULL));
    }

    // @TODO: Handle all options appropriately
    if (memcmp(request_type->chars, "GET", request_type->length) == 0) {
      curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    } else if (memcmp(request_type->chars, "POST", request_type->length) == 0) {
      curl_easy_setopt(curl, CURLOPT_POST, 1L);
    } else if (memcmp(request_type->chars, "HEAD", request_type->length) == 0) {
      curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
    } else {
      curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, request_type->chars);
    }

    // initialize mime form
    curl_mime *form = NULL;
    curl_mimepart *field = NULL;
    // initialize the headers...
    struct curl_slist *heads = NULL;

    // if request body is given
    if (!IS_NIL(args[11])) {
      form = curl_mime_init(curl);

      bool has_file = false;

      if (IS_DICT(args[11])) {

        b_obj_dict *request_body = AS_DICT(args[11]);

        if (request_body->names.count > 0) {

          /*char *input = "";*/

          for (int i = 0; i < request_body->names.count; i++) {
            b_obj_string *key = AS_STRING(request_body->names.values[i]);

            char *escaped_key = curl_easy_escape(curl, key->chars, key->length);

            if (escaped_key != NULL) {

              b_value val;
              if (dict_get_entry(request_body, request_body->names.values[i],
                                 &val)) {

                if (IS_FILE(val)) {
                  has_file = true;

                  field = curl_mime_addpart(form);
                  curl_mime_name(field, escaped_key);
                  curl_mime_filedata(field, realpath(AS_FILE(val)->path->chars, NULL));

                } else {
                  /*input = append_strings(input, escaped_key);
                  input = append_strings(input, "=");
                  free(escaped_key);*/

                  char *value = value_to_string(vm, val);

                  char *escaped_value =
                      curl_easy_escape(curl, value, (int)strlen(value));

                  if (escaped_value != NULL) {
                    /*input = append_strings(input, escaped_value);
                    free(escaped_value);*/
                    field = curl_mime_addpart(form);
                    curl_mime_name(field, escaped_key);
                    curl_mime_data(field, escaped_value, CURL_ZERO_TERMINATED);
                  }
                  /*input = append_strings(input, "&");*/
                }
              }
            }
          }

          if(has_file) {
            if(no_expect) {
              heads = curl_slist_append(heads, "Expect:");
            }
          }

          /*curl_easy_setopt(curl, CURLOPT_POSTFIELDS, input);*/
          curl_easy_setopt(form, CURLOPT_MIMEPOST, form);
        } else {
          curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "");
        }
      } else if (IS_STRING(args[11])) {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, AS_C_STRING(args[11]));
      }
    }

    // NOTE: Always set the user's headers after setting request body
    // this will enable the user to override curl's headers due to such
    // contents as files in the request body...
    if (headers->names.count > 0) {

      for (int i = 0; i < headers->names.count; i++) {
        b_obj_string *key = AS_STRING(headers->names.values[i]);
        b_value val;
        if (dict_get_entry(headers, headers->names.values[i], &val) &&
            IS_STRING(val)) {
          b_obj_string *value = AS_STRING(val);

          char *header_line = strdup(key->chars);
          header_line = append_strings(header_line, ": ");
          header_line = append_strings(header_line, value->chars);

          heads = curl_slist_append(heads, header_line);
        }
      }

      curl_easy_setopt(curl, CURLOPT_HTTPHEADER, heads);
    }

    FILE *stream = tmpfile();
    int header_length = 0;

    // GENERAL OPTIONS...
    curl_easy_setopt(curl, CURLOPT_HEADER, 1L);
    curl_easy_setopt(curl, CURLOPT_PROTOCOLS,
                     CURLPROTO_HTTP | CURLPROTO_HTTPS | CURLPROTO_FILE);
    // for writing the body
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, stream);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, http_module_write_header);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &header_length);

    CURLcode res_code = curl_easy_perform(curl);

    // we want to reset to the default in case we aren't going
    // to need a custom request in the next request. Else,
    // curl will keep using the custom request.
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, NULL);

    long status_code = 0;
    curl_easy_getinfo(curl, CURLINFO_HTTP_CODE, &status_code);

    b_value error = NIL_VAL;
    if (res_code != CURLE_OK) {
      const char *err = curl_easy_strerror(res_code);
      error = OBJ_VAL(copy_string(vm, err, (int)strlen(err)));
    }

    fseek(stream, 0L, SEEK_END);
    size_t stream_length = ftell(stream);
    rewind(stream);

    char *stream_content = (char *)malloc(stream_length);
    fread(stream_content, sizeof(char), stream_length, stream);

    fclose(stream);

    long redirect_count;
    double total_time;
    char *effective_url = NULL;
    curl_easy_getinfo(curl, CURLINFO_REDIRECT_COUNT, &redirect_count);
    curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &total_time);
    curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &effective_url);

    b_obj_string *header = copy_string(vm, stream_content, header_length);
    b_obj_string *body = copy_string(vm, stream_content + header_length,
                                     (int)stream_length - header_length);
    b_obj_string *responder =
        copy_string(vm, effective_url, (int)strlen(effective_url));

    // clean up
    curl_easy_cleanup(curl);
    curl_mime_free(form);
    curl_slist_free_all(heads);
//    free(effective_url);
//    free(stream_content);

    b_obj_list *list = new_list(vm);
    write_list(vm, list, NUMBER_VAL(status_code));
    write_list(vm, list, error);
    write_list(vm, list, OBJ_VAL(header));
    write_list(vm, list, OBJ_VAL(body));
    write_list(vm, list, NUMBER_VAL(total_time));
    write_list(vm, list, NUMBER_VAL(redirect_count));
    write_list(vm, list, OBJ_VAL(responder));

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