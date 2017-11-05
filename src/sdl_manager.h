#pragma once

#include <SDL2/SDL.h>

#include "pars.h"

#define WIN_TITLE "Dendulator"
#define WIN_WIDTH 256
#define WIN_HEIGHT 240

typedef void (*sdl_audio_callback_t)(void *, uint8_t *, int);
typedef void (*sdl_event_callback_t)(SDL_Event *, void *);

typedef struct {
  SDL_Window *win;
  SDL_Texture *tex;
  SDL_Renderer *ren;
} sdl_man_video_t;

typedef struct {
  int open;
  SDL_AudioSpec main_spec;
  SDL_AudioSpec obt_spec;
  SDL_AudioDeviceID dev;
  SDL_AudioStream *stream;
} sdl_man_audio_t;

typedef struct {
  SDL_Event ev;
  void *udata;
  sdl_event_callback_t callback;
} sdl_man_event_t;

typedef struct {
  sdl_man_audio_t a;
  sdl_man_video_t v;
  sdl_man_event_t e;
} sdl_man_t;

typedef SDL_Keycode sdl_key_t;

void sdl_init(sdl_man_t *sdl, pars_t *pars);
void sdl_cleanup(sdl_man_t *sdl);

void sdl_sleep(sdl_man_t *sdl, uint32_t ms);
void sdl_frame(sdl_man_t *sdl, uint32_t screen[240][256]);
uint32_t sdl_get_ticks(sdl_man_t *sdl);
void sdl_screenshot(sdl_man_t *sdl, const char *fname);

void sdl_set_event_callback(sdl_man_t *sdl, sdl_event_callback_t fn, void *ud);
void sdl_process_events(sdl_man_t *sdl);

void sdl_open_audio(sdl_man_t *sdl, int sn, sdl_audio_callback_t fn, void *ud);
void sdl_pause_audio(sdl_man_t *sdl, int pause);
void sdl_close_audio(sdl_man_t *sdl);
void sdl_mix_audio(sdl_man_t *sdl, uint8_t *buf, uint32_t buflen);
