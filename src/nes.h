#pragma once

#include "nes_structs.h"
#include "nes_cpu.h"
#include "nes_ppu.h"
#include "nes_apu.h"
#include "nes_mappers.h"
#include "pars.h"
#include "error.h"
#include "errcodes.h"
#include "nes_input.h"

#define NES_APU_SAMPLE_BUF_SIZE 4096 // audio buffer size (samples)

void nes_init(nes_t *nes, pars_t *pars);
void nes_cleanup(nes_t *nes);

void nes_load_rom(nes_t *nes, const char *fname);
void nes_unload_rom(nes_t *nes);

// main emulator tick function
// returns 1 if a frame is ready for display
static inline uint8_t nes_process(nes_t *nes) {
  uint32_t cycles = nes_cpu_op(nes); // step CPU
  // step everything else based on spent CPU cycles
  for (int i = 0; i < cycles; ++i) {
    nes_apu_tick(nes);
    nes_ppu_tick(nes);
    nes_mapper_tick(nes);
    nes_ppu_tick(nes);
    nes_mapper_tick(nes);
    nes_ppu_tick(nes);
    nes_mapper_tick(nes);
  }

  int render = BITGET(nes->ppu.flags, NES_PPU_FLAG_RENDER);
  if (render) nes->ppu.flags = BITCLR(nes->ppu.flags, NES_PPU_FLAG_RENDER);
  return render;
}
