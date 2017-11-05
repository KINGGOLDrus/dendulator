#include <stdio.h>

#include "error.h"
#include "errcodes.h"
#include "pars.h"
#include "core.h"

int main(int argc, char *argv[]) {
  static char *err_msg[] = {
    [ERR_ARGS] = "Incorrect arguments",
    [ERR_SDL_INIT] = "SDL initialization failed",
    [ERR_ROM_LOAD] = "ROM loading failed",
    [ERR_ROM_INIT] = "ROM mapper data initialization failed",
  };

  static error_t err;
  error_init(&err, err_msg);

  static pars_t pars;
  pars_parse(&pars, argc, argv);

  if (err.code != NO_ERR) {
    error_print_all(stderr, stderr);
    error_free_log();

    return err.code;
  }

  static core_t core;
  core_init(&core, &pars);

  if (err.code != NO_ERR) {
    error_print_all(stderr, stderr);
    error_free_log();

    return err.code;
  }

  core_load_rom(&core, pars.rom_fname);

  if (err.code == NO_ERR) {
    core_process(&core, &pars);
    core_unload_rom(&core);
  }

  error_print_all(stderr, stderr);
  error_free_log();

  core_cleanup(&core);

  return err.code;
}
