#include "nes.h"

#include "nes_cpu.h"
#include "nes_apu.h"
#include "nes_mem.h"
#include "nes_cart.h"
#include "nes_ppu.h"
#include "nes_input.h"

void nes_init(nes_t *nes, pars_t *pars) {
  nes_mem_init(&nes->mem);
  nes_apu_init(&nes->apu, NES_APU_SAMPLE_BUF_SIZE);
  nes_cpu_init(&nes->cpu);
  nes_vmem_init(&nes->vmem);
  nes_ppu_init(&nes->ppu);
  nes_input_init(&nes->input);
}

void nes_cleanup(nes_t *nes) {
  nes_apu_cleanup(&nes->apu);
  nes_ppu_cleanup(&nes->ppu);
}

void nes_load_rom(nes_t *nes, const char *fname) {
  nes_cart_load(nes, fname);
}

void nes_unload_rom(nes_t *nes) {
  nes_cart_unload(nes);
}
