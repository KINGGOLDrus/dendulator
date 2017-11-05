#include <string.h>

#include "error.h"
#include "errcodes.h"
#include "pars.h"

static inline void pars_set_default(pars_t *pars) {
  pars->rom_fname = NULL;

  pars->res_factor_w = 1;
  pars->res_factor_h = 1;

  pars->run_frames = 0;
}

static inline void pars_check(pars_t *pars) {
  if (!pars->rom_fname) {
    error_set_code(ERR_ARGS);
    error_log_write("ROM file name is not specified");
    return;
  }

  if ((pars->res_factor_w < PARS_RES_FACTOR_MIN) ||
      (pars->res_factor_w > PARS_RES_FACTOR_MAX)) {
    error_set_code(ERR_ARGS);
    error_log_write("Incorrect width resolution factor");
    return;
  }

  if ((pars->res_factor_h < PARS_RES_FACTOR_MIN) ||
      (pars->res_factor_h > PARS_RES_FACTOR_MAX)) {
    error_set_code(ERR_ARGS);
    error_log_write("Incorrect height resolution factor");
    return;
  }
}

void pars_parse(pars_t *pars, int argc, char *argv[]) {
  if (argc == 1) {
    error_set_code(ERR_ARGS);
    error_log_write("Not enough arguments");
    return;
  }

  pars_set_default(pars);

  int temp_int;
  for (int i = 1; i < argc;) {
    if (!strcmp(argv[i], "-w") || !strcmp(argv[i], "--width")) {
      if ((argc > i + 1) && sscanf(argv[i + 1], "%d", &temp_int)) {
        pars->res_factor_w = temp_int;
        i += 2;

        continue;
      }

      error_set_code(ERR_ARGS);
      error_log_write("Parameter -w (--width) requires integer value between "
        "1 and 5\n");
      return;
    }

    if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--height")) {
      if ((argc > i + 1) && sscanf(argv[i + 1], "%d", &temp_int)) {
        pars->res_factor_h = temp_int;
        i += 2;

        continue;
      }

      error_set_code(ERR_ARGS);
      error_log_write("Parameter -h (--height) requires integer value between "
        "1 and 5\n");
      return;
    }

    if (!strcmp(argv[i], "-x") || !strcmp(argv[i], "--scale")) {
      if ((argc >= i) && sscanf(argv[i + 1], "%d", &temp_int)) {
        pars->res_factor_w = temp_int;
        pars->res_factor_h = temp_int;
        i += 2;

        continue;
      }

      error_set_code(ERR_ARGS);
      error_log_write("Parameter -x (--scale) requires integer value between "
        "1 and 5\n");
      return;
    }

    if (!strcmp(argv[i], "-f") || !strcmp(argv[i], "--frames")) {
      if ((argc >= i) && sscanf(argv[i + 1], "%d", &temp_int)) {
        pars->run_frames = temp_int > 0 ? temp_int : 0;
        i += 2;

        continue;
      }

      error_set_code(ERR_ARGS);
      error_log_write("Parameter -f (--frames) requires positive integer "
        "value\n");
      return;
    }

    if (pars->rom_fname != NULL) {
      error_set_code(ERR_ARGS);
      error_log_write("ROM file name is specified already\n");
      return;
    }

    pars->rom_fname = argv[1];
    ++i;
  }

  pars_check(pars);
}
