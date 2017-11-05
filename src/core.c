#include "core.h"
#include "nes_apu.h"
#include "error.h"
#include "errcodes.h"

#include <math.h>

#if defined(DEBUG) && defined(DEBUG_SDL)
#include "sdl_debug.h"
#endif

#include "core_callbacks.h"

void core_load_rom(core_t *core, const char *fname) {
  nes_load_rom(&core->nes, fname);
}

void core_unload_rom(core_t *core) {
  nes_unload_rom(&core->nes);
}

static inline void core_state_init(core_state_t *state) {
  state->active_flag = 1;
}

void core_init_controls(core_controls_t *ctrls) {
  ctrls->p1 = (core_control_table_t){0};
  ctrls->p2 = (core_control_table_t){0};
}

void core_set_default_controls(core_controls_t *ctrls) {
  ctrls->p1.code[CTRLS_KEY_UP] = SDLK_UP;
  ctrls->p1.code[CTRLS_KEY_DOWN] = SDLK_DOWN;
  ctrls->p1.code[CTRLS_KEY_LEFT] = SDLK_LEFT;
  ctrls->p1.code[CTRLS_KEY_RIGHT] = SDLK_RIGHT;
  ctrls->p1.code[CTRLS_KEY_SELECT] = SDLK_z;
  ctrls->p1.code[CTRLS_KEY_START] = SDLK_x;
  ctrls->p1.code[CTRLS_KEY_B] = SDLK_a;
  ctrls->p1.code[CTRLS_KEY_A] = SDLK_s;

  ctrls->p2.code[CTRLS_KEY_UP] = SDLK_i;
  ctrls->p2.code[CTRLS_KEY_DOWN] = SDLK_k;
  ctrls->p2.code[CTRLS_KEY_LEFT] = SDLK_j;
  ctrls->p2.code[CTRLS_KEY_RIGHT] = SDLK_l;
  ctrls->p2.code[CTRLS_KEY_SELECT] = SDLK_BACKSLASH;
  ctrls->p2.code[CTRLS_KEY_START] = SDLK_RETURN;
  ctrls->p2.code[CTRLS_KEY_B] = SDLK_LEFTBRACKET;
  ctrls->p2.code[CTRLS_KEY_A] = SDLK_RIGHTBRACKET;
}

static void core_audio_feed(void *udata, uint8_t *buf, int buflen) {
  SDL_AudioStream **stream = udata;
  SDL_AudioStreamGet(*stream, buf, buflen);
}

static inline void core_open_audio(core_t *core, pars_t *pars) {
  sdl_open_audio(&core->sdl,
                 NES_APU_SAMPLE_BUF_SIZE / 2,
                 core_audio_feed,
                 &core->sdl.a.stream);
}

static inline void core_close_audio(core_t *core) {
  sdl_close_audio(&core->sdl);
}

void core_init(core_t *core, pars_t *pars) {
  sdl_init(&core->sdl, pars);

#if defined(DEBUG) && defined(DEBUG_SDL)
  sdl_debug_init();
#endif

  if (error_get_code() != NO_ERR)
    return;

  core_open_audio(core, pars);

  if (error_get_code() != NO_ERR) {
    sdl_cleanup(&core->sdl);
    return;
  }

  nes_init(&core->nes, pars);

  if (error_get_code() != NO_ERR) {
    sdl_close_audio(&core->sdl);
    sdl_cleanup(&core->sdl);
    return;
  }

  core_state_init(&core->state);

  core_init_controls(&core->ctrls);
  core_set_default_controls(&core->ctrls);

  sdl_set_event_callback(&core->sdl, core_proc_event, core);

  core->target_frame = pars->run_frames;
}

void core_cleanup(core_t *core) {
  core_close_audio(core);
  sdl_cleanup(&core->sdl);
  nes_cleanup(&core->nes);
}

void core_process(core_t *core, pars_t *pars) {
  static int64_t then, now;

  while (core->state.active_flag) {
    then = sdl_get_ticks(&core->sdl);

    sdl_process_events(&core->sdl);

    while (!nes_process(&core->nes)) {}

    if (core->nes.apu.buf_size > 0) {
      sdl_mix_audio(&core->sdl, core->nes.apu.buf, core->nes.apu.buf_size);
      core->nes.apu.buf_size = 0;
    }

#if defined(DEBUG) && defined(DEBUG_SDL)
    sdl_debug_frame(&core->nes);
#endif
    sdl_frame(&core->sdl, core->nes.ppu.front->data);

    now = sdl_get_ticks(&core->sdl);

    if (!core->target_frame) {
      if (now - then <= 16)
        sdl_sleep(&core->sdl, 16 - (now - then));
    } else if (core->nes.ppu.frame == core->target_frame) {
      core->state.active_flag = 0;
      sdl_screenshot(&core->sdl, "output.bmp");
    }
  }
}
