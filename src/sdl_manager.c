#include <string.h>

#include "sdl_manager.h"
#include "error.h"
#include "errcodes.h"

static inline void sdl_init_video(sdl_man_video_t *v, pars_t *pars) {
  if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0) {
    error_set_code(ERR_SDL_INIT);
    error_log_write("SDL video subsystem initialization failed\n");
    return;
  }

  v->win = SDL_CreateWindow(WIN_TITLE, SDL_WINDOWPOS_CENTERED,
                            SDL_WINDOWPOS_CENTERED,
                            WIN_WIDTH * pars->res_factor_w,
                            WIN_HEIGHT * pars->res_factor_h,
                            SDL_WINDOW_SHOWN);

  if (!v->win) {
    error_set_code(ERR_SDL_INIT);
    error_log_write("Could not create window\n");
    return;
  }

#ifndef SOFTWARE
  int flags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC;
#else
  int flags = 0;
#endif
  v->ren = SDL_CreateRenderer(v->win, -1, flags);

  if (!v->ren) {
    error_set_code(ERR_SDL_INIT);
    error_log_write("Could not create renderer\n");
    return;
  }

  SDL_SetRenderDrawColor(v->ren, 255, 0, 0, 255);

  v->tex = SDL_CreateTexture(v->ren,
                             SDL_PIXELFORMAT_ARGB8888,
                             SDL_TEXTUREACCESS_STREAMING,
                             256, 240);

  if (!v->tex) {
    error_set_code(ERR_SDL_INIT);
    error_log_write("Could not create texture\n");
    return;
  }
}

static inline void sdl_init_audio(sdl_man_audio_t *a, pars_t *pars) {
  if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
    error_set_code(ERR_SDL_INIT);
    error_log_write("SDL audio subsystem initialization failed\n");
  }
}

static inline void sdl_cleanup_video(sdl_man_video_t *v) {
  SDL_DestroyWindow(v->win);

  SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

static inline void sdl_cleanup_audio(sdl_man_audio_t *a) {
  SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

void sdl_init(sdl_man_t *sdl, pars_t *pars) {
  sdl_init_video(&sdl->v, pars);

  if (error_get_code() != NO_ERR)
    return;

  sdl_init_audio(&sdl->a, pars);

  if (error_get_code() != NO_ERR)
    sdl_cleanup_video(&sdl->v);
}

void sdl_open_audio(sdl_man_t *sdl, int sn, sdl_audio_callback_t fn, void *u) {
  sdl->a.main_spec.freq = 48000;
  sdl->a.main_spec.format = AUDIO_F32;
  sdl->a.main_spec.channels = 2;
  sdl->a.main_spec.samples = sn;
  sdl->a.main_spec.callback = fn;
  sdl->a.main_spec.userdata = u;

  SDL_AudioDeviceID dev = SDL_OpenAudioDevice(
    NULL, 0, &sdl->a.main_spec, &sdl->a.obt_spec,
    SDL_AUDIO_ALLOW_FORMAT_CHANGE
  );

  if (dev < 1) {
    error_set_code(ERR_SDL_INIT);
    error_log_write("SDL audio could not open\n");
    return;
  }

  sdl->a.dev = dev;
  sdl->a.stream = SDL_NewAudioStream(
    AUDIO_U8, 1, 48000, 
    sdl->a.obt_spec.format, sdl->a.obt_spec.channels, sdl->a.obt_spec.freq
  );

  if (!sdl->a.stream) {
    error_set_code(ERR_SDL_INIT);
    error_log_write("SDL audio stream could not open\n");
    return;
  }

  sdl->a.open = 1;
  SDL_PauseAudioDevice(dev, 0);
}

void sdl_mix_audio(sdl_man_t *sdl, uint8_t *buf, uint32_t buflen) {
  if (!sdl->a.open) return;
  SDL_AudioStreamPut(sdl->a.stream, buf, buflen);
}

void sdl_frame(sdl_man_t *sdl, uint32_t screen[240][256]) {
  void *raw_pixels = NULL;
  int pitch = 0;

  SDL_LockTexture(sdl->v.tex, NULL, &raw_pixels, &pitch);
  memcpy(raw_pixels, screen, pitch * 240);
  SDL_UnlockTexture(sdl->v.tex);

  SDL_SetRenderDrawColor(sdl->v.ren, 255, 0, 0, 255);
  SDL_RenderClear(sdl->v.ren);
  SDL_SetRenderDrawColor(sdl->v.ren, 255, 255, 255, 255);
  SDL_RenderCopyEx(sdl->v.ren, sdl->v.tex, NULL, NULL, 0, NULL, SDL_FLIP_NONE);
  SDL_RenderPresent(sdl->v.ren);
}

void sdl_sleep(sdl_man_t *sdl, uint32_t ms) {
  SDL_Delay(ms);
}

uint32_t sdl_get_ticks(sdl_man_t *sdl) {
  return SDL_GetTicks();
}

void sdl_screenshot(sdl_man_t *sdl, const char *fname) {
  int w, h;
  SDL_GetRendererOutputSize(sdl->v.ren, &w, &h);
  SDL_Surface *sshot =
    SDL_CreateRGBSurface(0, w, h, 32,
                         0x00FF0000, 0x0000FF00, 0x000000FF, 0x00000000);
  if (!sshot) return;
  SDL_RenderReadPixels(sdl->v.ren, NULL, SDL_PIXELFORMAT_RGB888,
                       sshot->pixels, sshot->pitch);
  SDL_SaveBMP(sshot, fname);
  SDL_FreeSurface(sshot);
}

void sdl_set_event_callback(sdl_man_t *sdl, sdl_event_callback_t fn, void *u) {
  sdl->e.callback = fn;
  sdl->e.udata = u;
}

void sdl_process_events(sdl_man_t *sdl) {
  while (SDL_PollEvent(&sdl->e.ev))
    sdl->e.callback(&sdl->e.ev, sdl->e.udata);
}

void sdl_close_audio(sdl_man_t *sdl) {
  if (!sdl->a.open) return;
  sdl->a.open = 0;
  SDL_PauseAudioDevice(sdl->a.dev, 1);
  SDL_CloseAudioDevice(sdl->a.dev);
}

void sdl_cleanup(sdl_man_t *sdl) {
  sdl_cleanup_audio(&sdl->a);
  sdl_cleanup_video(&sdl->v);
}
