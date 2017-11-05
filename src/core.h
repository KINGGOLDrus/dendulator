#pragma once

#include "pars.h"
#include "sdl_manager.h"
#include "nes.h"

// "core" basically means "i/o glue"
// rewrite this part when porting to a different platform

#define CTRLS_KEY_COUNT 8

// keybind indices
enum ctrls_key_code {
  CTRLS_KEY_UP,
  CTRLS_KEY_DOWN,
  CTRLS_KEY_LEFT,
  CTRLS_KEY_RIGHT,
  CTRLS_KEY_SELECT,
  CTRLS_KEY_START,
  CTRLS_KEY_B,
  CTRLS_KEY_A,
};

// keybind table
typedef struct {
  sdl_key_t code[CTRLS_KEY_COUNT];
} core_control_table_t;

// all the keybindings
typedef struct {
  core_control_table_t p1;
  core_control_table_t p2;
} core_controls_t;

// core state flags
typedef struct {
  char active_flag;
} core_state_t;

// core state struct
typedef struct {
  sdl_man_t sdl;
  nes_t nes;

  uint32_t target_frame;

  core_state_t state;
  core_controls_t ctrls;
} core_t;

void core_load_rom(core_t *core, const char *fname);
void core_unload_rom(core_t *core);

void core_init(core_t *core, pars_t *pars);
void core_cleanup(core_t *core);
void core_set_default_controls(core_controls_t *ctrls);
void core_init_controls(core_controls_t *ctrls);

void core_process(core_t *core, pars_t *pars);
