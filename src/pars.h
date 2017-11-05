#pragma once

#include <stdio.h>

#define PARS_RES_FACTOR_MIN 1
#define PARS_RES_FACTOR_MAX 5

typedef struct {
  char *rom_fname;

  unsigned char res_factor_w;
  unsigned char res_factor_h;

  unsigned int run_frames;
} pars_t;

void pars_parse(pars_t *pars, int argc, char *argv[]);