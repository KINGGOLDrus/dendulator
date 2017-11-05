#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdarg.h>

#include "error.h"
#include "nes_structs.h"
#include "bitops.h"

static SDL_Window *sdl_debug_win;
static SDL_Renderer *sdl_debug_ren;
static SDL_Texture *sdl_font;
static int sdl_font_ch[257][2];
static const int sdl_font_char_w = 8;
static const int sdl_font_char_h = 8;

static void sdl_debug_init(void) {
  sdl_debug_win = SDL_CreateWindow("Debug Info", SDL_WINDOWPOS_CENTERED,
                            SDL_WINDOWPOS_CENTERED,
                            320, 128,
                            SDL_WINDOW_SHOWN);

  if (!sdl_debug_win) {
    error_log_write("Could not create debug window\n");
    return;
  }

  sdl_debug_ren = SDL_CreateRenderer(sdl_debug_win, -1, 0);

  if (!sdl_debug_ren) {
    error_log_write("Could not create debug renderer\n");
    return;
  }

  for (int i = 0; i < 256; ++i) {
    sdl_font_ch[i][0] = sdl_font_char_w*(i % 16);
    sdl_font_ch[i][1] = sdl_font_char_h*(i / 16);
  }

  sdl_font_ch[256][0] = sdl_font_char_w;
  sdl_font_ch[256][1] = sdl_font_char_h;

  static char fname_buf[1024];
  snprintf(fname_buf, 1024, "%s/font.bmp", SDL_GetBasePath());
  SDL_Surface *sur = SDL_LoadBMP(fname_buf);
  if (!sur) {
    error_log_write("Could not load debug font\n");
    return;
  }

  sdl_font = SDL_CreateTextureFromSurface(sdl_debug_ren, sur);
  if (!sdl_font) {
    error_log_write("Could not create debug font texture\n");
  }

  SDL_FreeSurface(sur);
}

static void sdl_debug_print(int x, int y, char *str) {
  if (!str || !sdl_font) return;
  register const char *p = str;
  SDL_Rect dst = {.x = x, .y = y,
                  .w = sdl_font_ch[256][0], .h = sdl_font_ch[256][1]};
  SDL_Rect src = {.x = 0, .y = 0,
                  .w = sdl_font_ch[256][0], .h = sdl_font_ch[256][1]};
  while (*p != '\0') {
    if (*p == '\n')  {
      dst.x = x;
      dst.y += src.h;
    } else  {
      src.x = sdl_font_ch[(uint8_t)*p][0];
      src.y = sdl_font_ch[(uint8_t)*p][1];
      SDL_RenderCopy(sdl_debug_ren, sdl_font, &src, &dst);
      dst.x += sdl_font_ch[256][0];
    }
    ++p;
  }
}

static void sdl_debug_printf(int x, int y, const char *fmt, ...) {
  static char buf[1024];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, 1024, fmt, args);
  va_end(args);
  sdl_debug_print(x, y, buf);
}

static void sdl_debug_frame(nes_t *nes) {
  SDL_SetRenderDrawColor(sdl_debug_ren, 0, 0, 0, 255);
  SDL_RenderClear(sdl_debug_ren);
  SDL_SetRenderDrawColor(sdl_debug_ren, 255, 255, 255, 255);
  char flags[9] = "NVssDIZC";
  for (int i = 7; i >= 0; --i)
    if (!BITGET(nes->cpu.p, i)) flags[i] = '-';
  char bkgpal[33] = {0};
  char sprpal[33] = {0};
  for (int i = 0; i < 16; ++i) {
    sprintf(bkgpal + i * 2, "%02X", nes->vmem.pal[i]);
    sprintf(sprpal + i * 2, "%02X", nes->vmem.pal[i + 16]);
  }
  sdl_debug_printf(8, 8, "FRAME %08d CYCLE %llu\n\nPC=%04X P=%s"
                   "S=%02X A=%02X X=%02X Y=%02X\n\n"
                   "$2000=%02X $2001=%02X $2002=%02X $2003=%02X\n"
                   "VADDR=%04X TADDR=%04X FLAGS=%02X BUS=%02X\n\n"
                   "BGPAL=%s\nSPPAL=%s\n\n"
                   "P1CUR=%02X P1SAV=%02X P2CUR=%02X P2SAV=%02X\n",
                   nes->ppu.frame, nes->cpu.cycle, nes->cpu.pc, flags, nes->cpu.s,
                   nes->cpu.a, nes->cpu.x, nes->cpu.y, nes->ppu.ctrl, nes->ppu.mask,
                   nes->ppu.status, nes->ppu.oam_addr, nes->ppu.vmem_addr,
                   nes->ppu.tmp_addr, nes->ppu.flags, nes->ppu.bus, bkgpal, sprpal,
                   nes->input.p1.cur.btns, nes->input.p1.saved.btns,
                   nes->input.p2.cur.btns, nes->input.p2.saved.btns);
  SDL_RenderPresent(sdl_debug_ren);
}
