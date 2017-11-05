#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// error log struct
typedef struct {
  char *text; // error string
  size_t size; // error string length
} error_log_t;

// error logger struct
typedef struct {
  int code; // last error code
  error_log_t log; // error log
} error_t;

void error_init(error_t *err_ptr, char *err_msg_tbl_ptr[]);
void error_log_init(error_log_t *log);

void error_print_msg(FILE *msg_stream);
void error_print_log(FILE *log_stream);
void error_print_all(FILE *msg_stream, FILE *log_stream);

int error_get_code();
int *error_get_code_ptr();
void error_set_code(int code);

size_t error_log_write(const char *text);
void error_free_log();
