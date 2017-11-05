#include <stdio.h>

#include "nes_cart.h"
#include "nes_mappers.h"
#include "nes_cpu.h"
#include "nes_mem.h"
#include "error.h"
#include "errcodes.h"

// video memory address translation functions for different mirroring modes

// single screen (page 0)
uint16_t nes_cart_mirror_single0(uint16_t addr) {
  return addr & 0x03FF;
}

// single screen (page 1)
uint16_t nes_cart_mirror_single1(uint16_t addr) {
  return 0x400 + (addr & 0x03FF);
}

// no mirroring (2x2 pages)
uint16_t nes_cart_mirror_none(uint16_t addr) {
  return addr & 0x0FFF;
}

// horizontal mirroring
uint16_t nes_cart_mirror_hor(uint16_t addr) {
  return (addr < 0x2800) ? (addr & 0x03FF) : ((addr & 0x03FF) | 0x0400);
}

// vertical mirroring
uint16_t nes_cart_mirror_vert(uint16_t addr) {
  return addr & 0x07FF;
}

static const nes_mirror_func_t nes_cart_mirrors[] = {
  nes_cart_mirror_vert,
  nes_cart_mirror_hor,
  nes_cart_mirror_none,
  nes_cart_mirror_single0,
  nes_cart_mirror_single1,
};

void nes_cart_set_mirroring(nes_t *nes, enum mirror_mode mode) {
  nes->cart.mirror = nes_cart_mirrors[mode];
}

enum mirror_mode nes_cart_get_mirroring(nes_t *nes) {
  for (int m = MIRROR_NONE; m < MIRROR_CUSTOM; m++) {
    if (nes->cart.mirror == nes_cart_mirrors[m])
      return m;
  }
  return MIRROR_CUSTOM;
}

// rom functions

// reads an iNES ROM from stream
static inline void nes_cart_read_rom(nes_t *nes, FILE *src) {
  rewind(src);

  if (!(fgetc(src) == 'N' && fgetc(src) == 'E' && fgetc(src) == 'S' &&
        fgetc(src) == '\32')) {
    error_set_code(ERR_ROM_LOAD);
    error_log_write("Corrupted ROM file\n");
    return;
  }

  uint8_t rom16_count = fgetc(src);
  uint8_t vram8_count = fgetc(src);
  uint8_t ctrlbyte = fgetc(src);
  uint8_t mapper = fgetc(src) | (ctrlbyte >> 4);

  fgetc(src); fgetc(src); fgetc(src); fgetc(src);
  fgetc(src); fgetc(src); fgetc(src); fgetc(src);

  if (mapper > 0x40) mapper &= 0x0F;

  if (rom16_count) {
    if (!(nes->cart.rom = malloc(sizeof(uint8_t *) * rom16_count))) {
      error_set_code(ERR_ROM_LOAD);
      error_log_write("Out of memory on ROM reading!\n");
      return;
    }

    for (int i = 0; i < rom16_count; ++i) {
      if (!(nes->cart.rom[i] = malloc(0x4000))) {
        for (int j = 0; j < i; ++j)
          free(nes->cart.rom[j]);

        free(nes->cart.rom);

        error_set_code(ERR_ROM_LOAD);
        error_log_write("Out of memory on ROM bank reading!\n");
        return;
      }
    }
  }

  if (!vram8_count) {
    // one 8k CHR-RAM present
    vram8_count = 1;
    nes->cart.chr_ram = 1;
  }

  if (vram8_count) {
    if (!(nes->cart.vram = malloc(sizeof(uint8_t *) * vram8_count))) {
      error_set_code(ERR_ROM_LOAD);
      error_log_write("Out of memory on VRAM reading!\n");
      return;
    }

    for (int i = 0; i < vram8_count; ++i) {
      if (!(nes->cart.vram[i] = malloc(0x2000))) {
        for (int j = 0; j < i; ++j)
          free(nes->cart.vram[j]);

        free(nes->cart.vram);

        for (int j = 0; j < rom16_count; ++j)
          free(nes->cart.rom[j]);

        free(nes->cart.rom);

        error_set_code(ERR_ROM_LOAD);
        error_log_write("Out of memory on VRAM bank reading!\n");
        return;
      }
    }
  }

  for (int i = 0; i < rom16_count; ++i)
    fread(nes->cart.rom[i], 1, 0x4000, src);

  for (int i = 0; i < vram8_count; ++i)
    fread(nes->cart.vram[i], 1, 0x2000, src);

  nes->cart.rom16_count = rom16_count;
  nes->cart.vram8_count = vram8_count;

  if (BITGET(ctrlbyte, 4))
    nes_cart_set_mirroring(nes, MIRROR_NONE);
  else if (BITGET(ctrlbyte, 0))
    nes_cart_set_mirroring(nes, MIRROR_VERTICAL);
  else
    nes_cart_set_mirroring(nes, MIRROR_HORIZONTAL);

  nes_get_mapper_funcs(mapper, &nes->cart.mapper.funcs);

  fprintf(stdout, "%d 16KB ROM, %d 8KB VR%cM, Mapper %d (%s), CTRL %d\n", rom16_count,
          vram8_count, nes->cart.chr_ram ? 'A' : 'O', mapper, nes_get_mapper_name(mapper), ctrlbyte);
}


// attempts to load the given ROM file
void nes_cart_load(nes_t *nes, const char *fname) {
  FILE *src = fopen(fname, "rb");

  if (!src) {
    error_set_code(ERR_ROM_LOAD);
    error_log_write("ROM file not found\n");
    return;
  }

  nes_cart_read_rom(nes, src);

  fclose(src);

  if (error_get_code() != NO_ERR)
    return;

  nes_mapper_init(nes);

  if (error_get_code() != NO_ERR)
    return;

  fprintf(stdout, "VEC_NMI: %04X, VEC_RESET: %04X, VEC_IRQ: %04X\n",
          nes_mem_readw(nes, NES_VEC_NMI),
          nes_mem_readw(nes, NES_VEC_RESET),
          nes_mem_readw(nes, NES_VEC_IRQ));

  nes->cpu.pc = nes_mem_readw(nes, NES_VEC_RESET);
}

// frees rom banks
static inline void nes_cart_free_rom(nes_t *nes) {
  for (int i = 0; i < nes->cart.rom16_count; ++i)
    free(nes->cart.rom[i]);

  free(nes->cart.rom);
}

// frees video memory banks
static inline void nes_cart_free_vram(nes_t *nes) {
  for (int i = 0; i < nes->cart.vram8_count; ++i)
    free(nes->cart.vram[i]);

  free(nes->cart.vram);
}

void nes_cart_unload(nes_t *nes) {
  nes_mapper_cleanup(nes);
  nes_cart_free_rom(nes);
  nes_cart_free_vram(nes);
}
