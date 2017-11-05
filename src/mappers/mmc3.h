#pragma once

#include "../nes_mappers.h"
#include "../nes_mem.h"
#include "../nes_apu.h"
#include "../nes_cpu.h"
#include "../nes_input.h"
#include "../nes_ppu.h"

#define NES_MAPPER_ID_MMC3 4

// extra mapper data for MMC3
typedef struct {
  uint8_t reg_idx;
  uint8_t reg[8];
  uint8_t prg_mode;
  uint8_t chr_mode;
  uint32_t prg_offset[4];
  uint32_t chr_offset[8];
  uint8_t reload;
  uint8_t counter;
  uint8_t irq;
} nes_mmc3_extra_t;

// calculates MMC3 PRG bank offset for a given index
static inline uint32_t nes_mmc3_prg_offset(nes_t *nes, int32_t idx) {
  if (idx >= 0x80) idx -= 0x100;
  idx %= nes->cart.rom16_count * 2;
  int32_t offset = idx * 0x2000;
  if (offset < 0) offset += nes->cart.rom16_count * 0x4000;
  return offset;
}

// calculates MMC3 CHR bank offset for a given index
static inline uint32_t nes_mmc3_chr_offset(nes_t *nes, int32_t idx) {
  if (idx >= 0x80) idx -= 0x100;
  idx %= nes->cart.vram8_count * 8;
  int32_t offset = idx * 0x0400;
  if (offset < 0) offset += nes->cart.vram8_count * 0x2000;
  return offset;
}

// updates bank offsets
static inline void nes_mmc3_update_offsets(nes_t *nes) {
  nes_mmc3_extra_t *mmc = nes->cart.mapper.extra;
  switch (mmc->prg_mode) {
    case 0:
      mmc->prg_offset[0] = nes_mmc3_prg_offset(nes, mmc->reg[6]);
      mmc->prg_offset[1] = nes_mmc3_prg_offset(nes, mmc->reg[7]);
      mmc->prg_offset[2] = nes_mmc3_prg_offset(nes, -2);
      mmc->prg_offset[3] = nes_mmc3_prg_offset(nes, -1);
      break;
    case 1:
      mmc->prg_offset[2] = nes_mmc3_prg_offset(nes, mmc->reg[6]);
      mmc->prg_offset[1] = nes_mmc3_prg_offset(nes, mmc->reg[7]);
      mmc->prg_offset[0] = nes_mmc3_prg_offset(nes, -2);
      mmc->prg_offset[3] = nes_mmc3_prg_offset(nes, -1);
      break;
  }

  switch (mmc->chr_mode) {
    case 0:
      mmc->chr_offset[0] = nes_mmc3_chr_offset(nes, mmc->reg[0] & 0xFE);
      mmc->chr_offset[1] = nes_mmc3_chr_offset(nes, mmc->reg[0] | 0x01);
      mmc->chr_offset[2] = nes_mmc3_chr_offset(nes, mmc->reg[1] & 0xFE);
      mmc->chr_offset[3] = nes_mmc3_chr_offset(nes, mmc->reg[1] | 0x01);
      mmc->chr_offset[4] = nes_mmc3_chr_offset(nes, mmc->reg[2]);
      mmc->chr_offset[5] = nes_mmc3_chr_offset(nes, mmc->reg[3]);
      mmc->chr_offset[6] = nes_mmc3_chr_offset(nes, mmc->reg[4]);
      mmc->chr_offset[7] = nes_mmc3_chr_offset(nes, mmc->reg[5]);
      break;
    case 1:
      mmc->chr_offset[4] = nes_mmc3_chr_offset(nes, mmc->reg[0] & 0xFE);
      mmc->chr_offset[5] = nes_mmc3_chr_offset(nes, mmc->reg[0] | 0x01);
      mmc->chr_offset[6] = nes_mmc3_chr_offset(nes, mmc->reg[1] & 0xFE);
      mmc->chr_offset[7] = nes_mmc3_chr_offset(nes, mmc->reg[1] | 0x01);
      mmc->chr_offset[0] = nes_mmc3_chr_offset(nes, mmc->reg[2]);
      mmc->chr_offset[1] = nes_mmc3_chr_offset(nes, mmc->reg[3]);
      mmc->chr_offset[2] = nes_mmc3_chr_offset(nes, mmc->reg[4]);
      mmc->chr_offset[3] = nes_mmc3_chr_offset(nes, mmc->reg[5]);
      break;
  }
}

// checks for and triggers scanline IRQ
static inline void nes_mmc3_scanline(nes_t *nes) {
  nes_mmc3_extra_t *mmc = nes->cart.mapper.extra;
  if (mmc->counter == 0) {
    mmc->counter = mmc->reload;
  } else {
    mmc->counter--;
    if (mmc->counter == 0 && mmc->irq)
      nes_cpu_irq(nes);
  }
}

// writes to MMC3 BANKSELECT register
static inline void nes_mmc3_write_bankselect(nes_t *nes, uint8_t val) {
  nes_mmc3_extra_t *mmc = nes->cart.mapper.extra;
  mmc->prg_mode = (val >> 6) & 0x01;
  mmc->chr_mode = (val >> 7) & 0x01;
  mmc->reg_idx = val & 0x07;
  nes_mmc3_update_offsets(nes);
}

// writes to MMC3 BANKDATA register
static inline void nes_mmc3_write_bankdata(nes_t *nes, uint8_t val) {
  nes_mmc3_extra_t *mmc = nes->cart.mapper.extra;
  mmc->reg[mmc->reg_idx] = val;
  nes_mmc3_update_offsets(nes);
}

// writes to MMC3 MIRROR register
static inline void nes_mmc3_write_mirror(nes_t *nes, uint8_t val) {
  if (val & 0x01)
    nes_cart_set_mirroring(nes, MIRROR_HORIZONTAL);
  else
    nes_cart_set_mirroring(nes, MIRROR_VERTICAL);
}

// writes to the MMC3 register at given address
static inline void nes_mmc3_write(nes_t *nes, uint16_t addr, uint8_t val) {
  nes_mmc3_extra_t *mmc = nes->cart.mapper.extra;
  uint16_t f = addr & 0x01;
  if (addr <= 0x9FFF) {
    if (f) nes_mmc3_write_bankdata(nes, val);
    else nes_mmc3_write_bankselect(nes, val);
  } else if (addr <= 0xBFFF) {
    if (f) /*nes_mmc3_write_protect(nes, val)*/;
    else nes_mmc3_write_mirror(nes, val);
  } else if (addr <= 0xDFFF) {
    if (f) mmc->counter = 0;
    else mmc->reload = val;
  } else if (addr <= 0xFFFF) {
    if (f) mmc->irq = 1;
    else mmc->irq = 0;
  }
}

static uint8_t nes_mem_read_mmc3(nes_t *nes, uint16_t addr) {
  nes_mmc3_extra_t *mmc = nes->cart.mapper.extra;
  if (addr < 0x2000) return nes_ram_read(nes, addr & 0x07FF);
  if ((addr == 0x4016) || (addr == 0x4017))
    return nes_input_read(nes, addr - 0x4016);
  if (addr < 0x4000) return nes_ppu_read(nes, addr & 0x0007);
  if (addr < 0x4020) return nes_apu_read(nes, addr - 0x4000);
  if (addr >= 0x8000) {
    addr -= 0x8000;
    uint16_t bank8 = addr / 0x2000;
    uint16_t offset = addr & 0x1FFF;
    uint16_t bank16 = mmc->prg_offset[bank8] / 0x4000;
    uint16_t bank8_base = 0x2000 * ((mmc->prg_offset[bank8] / 0x2000) & 0x01);
    return nes->cart.rom[bank16][bank8_base + offset];
  }
  if (addr >= 0x6000) return nes_prgram_read(nes, addr - 0x6000);

  return 0x00;
}

static void nes_mem_write_mmc3(nes_t *nes, uint16_t addr, uint8_t val) {
  if (addr < 0x2000) {nes_ram_write(nes, addr & 0x07FF, val); return;}
  if (addr < 0x4000) {nes_ppu_write(nes, addr & 0x0007, val); return;}
  if (addr == 0x4014) {nes_ppu_oamdma(nes, val); return;}
  if (addr == 0x4016) {
    nes_input_write(nes, addr - 0x4016, val);
    return;
  }
  if (addr < 0x4020) {nes_apu_write(nes, addr - 0x4000, val); return;}
  if (addr >= 0x8000) {nes_mmc3_write(nes, addr, val); return;}
  if (addr >= 0x6000) {nes_prgram_write(nes, addr - 0x6000, val); return;}
}

static uint8_t nes_vmem_read_mmc3(nes_t *nes, uint16_t addr) {
  nes_mmc3_extra_t *mmc = nes->cart.mapper.extra;
  addr &= 0x3FFF;
  if (addr >= 0x3F00) {
    addr &= 0x001F;
    if (addr >= 0x10 && (addr & 0x03) == 0)
      addr -= 0x10;
    return nes->vmem.pal[addr];
  }
  if (addr < 0x2000) {
    uint16_t bank1 = addr / 0x0400;
    uint16_t offset = addr % 0x0400;
    uint16_t bank8 = mmc->chr_offset[bank1] / 0x2000;
    uint16_t bank1_base = 0x0400 * ((mmc->chr_offset[bank1] / 0x0400) & 0x07);
    return nes->cart.vram[bank8][bank1_base + offset];
  }
  return nes->vmem.vram[nes->cart.mirror(addr)];
}

static void nes_vmem_write_mmc3(nes_t *nes, uint16_t addr, uint8_t val) {
  nes_mmc3_extra_t *mmc = nes->cart.mapper.extra;
  addr &= 0x3FFF;
  if (addr >= 0x3F00) {
    addr &= 0x001F;
    if (addr >= 0x10 && (addr & 0x03) == 0)
      addr -= 0x10;
    nes->vmem.pal[addr] = val;
  } else if (addr < 0x2000) {
    if (!nes->cart.chr_ram) return;
    uint16_t bank1 = addr / 0x0400;
    uint16_t offset = addr % 0x0400;
    uint16_t bank8 = mmc->chr_offset[bank1] / 0x2000;
    uint16_t bank1_base = 0x0400 * ((mmc->chr_offset[bank1] / 0x0400) & 0x07);
    nes->cart.vram[bank8][bank1_base + offset] = val;
  } else {
    nes->vmem.vram[nes->cart.mirror(addr)] = val;
  }
}

static void nes_init_mmc3(nes_t *nes) {
  nes_mmc3_extra_t *mmc = calloc(1, sizeof(nes_mmc3_extra_t));
  nes->cart.mapper.extra = mmc;
  mmc->prg_offset[0] = nes_mmc3_prg_offset(nes, 0);
  mmc->prg_offset[1] = nes_mmc3_prg_offset(nes, 1);
  mmc->prg_offset[2] = nes_mmc3_prg_offset(nes, -2);
  mmc->prg_offset[3] = nes_mmc3_prg_offset(nes, -1);
  nes->mem.prg[0] = NULL;
  nes->mem.prg[1] = NULL;
}

static void nes_tick_mmc3(nes_t *nes) {
  if (nes->ppu.cycle != 260)
    return;
  if (nes->ppu.scanline > 239 && nes->ppu.scanline < 261)
    return;
  if (!BITGET(nes->ppu.mask, NES_PPU_MASK_BG) &&
      !BITGET(nes->ppu.mask, NES_PPU_MASK_SPR))
    return;
  nes_mmc3_scanline(nes);
}

static void nes_cleanup_mmc3(nes_t *nes) {
  free(nes->cart.mapper.extra);
  nes->cart.mapper.extra = NULL;
}

// registry stuff

MAPPER_REG_FUNC
static void nes_register_mmc3() {
  static const char *mapper_name = "MMC3";
  static nes_mapper_funcs_t mapper_funcs = {
    .init = nes_init_mmc3, .cleanup = nes_cleanup_mmc3,
    .read = nes_mem_read_mmc3, .write = nes_mem_write_mmc3,
    .vread = nes_vmem_read_mmc3, .vwrite = nes_vmem_write_mmc3,
    .tick = nes_tick_mmc3,
  };

  nes_reg_mapper(NES_MAPPER_ID_MMC3, mapper_name, &mapper_funcs);
}

MAPPER_UNREG_FUNC
static void nes_unregister_mmc3() {
  nes_unreg_mapper(NES_MAPPER_ID_MMC3);
}
