#include "error.h"

static error_t *err;
static char **err_msg_tbl;

static inline void error_log_init_inner(error_log_t *log) {
  log->text = NULL;
  log->size = 0;
}

void error_init(error_t *err_ptr, char *err_msg_tbl_ptr[]) {
  err = err_ptr;
  err_msg_tbl = err_msg_tbl_ptr;

  err->code = 0;
  error_log_init_inner(&err->log);
}

void error_log_init(error_log_t *log) {
  error_log_init_inner(log);
}

static inline void error_print_msg_inner(FILE *msg_stream) {
  if (err->code != 0)
    fprintf(msg_stream, "%s\n\n", err_msg_tbl[err->code]);
}

static inline void error_print_log_inner(FILE *log_stream) {
  if ((err->log.text != NULL) && (err->log.size > 1))
    fprintf(log_stream, "%s\n", err->log.text);
}

void error_print_msg(FILE *msg_stream) {
  error_print_msg_inner(msg_stream);
}

void error_print_log(FILE *log_stream) {
  error_print_log_inner(log_stream);
}

void error_print_all(FILE *msg_stream, FILE *log_stream) {
  error_print_msg_inner(msg_stream);
  error_print_log_inner(log_stream);
}

int error_get_code() {
  return err->code;
}

int *error_get_code_ptr() {
  return &err->code;
}

void error_set_code(int code) {
  err->code = code;
}

size_t error_log_write(const char *text) {
  size_t len = strlen(text) + 1;

  if (err->log.text) {
    size_t log_size = err->log.size - 1;
    char *tmp = malloc(log_size + len);

    if (tmp) {
      memcpy(tmp, err->log.text, log_size);
      memcpy(tmp + log_size, text, len);

      free(err->log.text);
      err->log.text = tmp;
      err->log.size = log_size + len;

      return len;
    }

    return 0;
  }

  if ((err->log.text = malloc(len))) {
    memcpy(err->log.text, text, len);
    err->log.size = len;

    return len;
  }

  return 0;
}

void error_free_log() {
  if (err->log.text)
    free(err->log.text);
}
