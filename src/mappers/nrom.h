#pragma once

#include "../nes_mappers.h"
#include "../nes_mem.h"
#include "../nes_apu.h"
#include "../nes_input.h"
#include "../nes_ppu.h"

#define NES_MAPPER_ID_NROM 0

static uint8_t nes_mem_read_nrom(nes_t *nes, uint16_t addr) {
  if (addr < 0x2000) return nes_ram_read(nes, addr & 0x07FF);
  if ((addr == 0x4016) || (addr == 0x4017))
    return nes_input_read(nes, addr - 0x4016);
  if (addr < 0x4000) return nes_ppu_read(nes, addr & 0x0007);
  if (addr < 0x4020) return nes_apu_read(nes, addr - 0x4000);
  if (addr >= 0xC000) return nes_prg_read(nes, 0, addr - 0xC000);
  if (addr >= 0x8000) return nes_prg_read(nes, 1, addr - 0x8000);
  if (addr >= 0x6000) return nes_prgram_read(nes, addr - 0x6000);

  return 0x00;
}

static void nes_mem_write_nrom(nes_t *nes, uint16_t addr, uint8_t val) {
  if (addr < 0x2000) {nes_ram_write(nes, addr & 0x07FF, val); return;}
  if (addr < 0x4000) {nes_ppu_write(nes, addr & 0x0007, val); return;}
  if (addr == 0x4014) {nes_ppu_oamdma(nes, val); return;}
  if (addr == 0x4016) {
    nes_input_write(nes, addr - 0x4016, val);
    return;
  }
  if (addr < 0x4020) {nes_apu_write(nes, addr - 0x4000, val); return;}
  if (addr >= 0x6000) {nes_prgram_write(nes, addr - 0x6000, val); return;}
}

static uint8_t nes_vmem_read_nrom(nes_t *nes, uint16_t addr) {
  addr &= 0x3FFF;
  if (addr >= 0x3F00) {
    addr &= 0x001F;
    if (addr >= 0x10 && (addr & 0x03) == 0)
      addr -= 0x10;
    return nes->vmem.pal[addr];
  }
  // NROM always has two 4k charbanks or none
  if (addr < 0x2000) {
    if (nes->cart.vram8_count)
      return nes->cart.vram[0][addr & 0x1FFF];
    else
      return 0x00;
  }
  return nes->vmem.vram[nes->cart.mirror(addr)];
}

static void nes_vmem_write_nrom(nes_t *nes, uint16_t addr, uint8_t val) {
  addr &= 0x3FFF;
  if (addr >= 0x3F00) {
    addr &= 0x001F;
    if (addr >= 0x10 && (addr & 0x03) == 0)
      addr -= 0x10;
    nes->vmem.pal[addr] = val;
  } else if (addr < 0x2000) {
    // NROM can have two 4k CHR-RAM banks
    if (nes->cart.vram8_count && nes->cart.chr_ram)
      nes->cart.vram[0][addr & 0x1FFF] = val;
  } else {
    nes->vmem.vram[nes->cart.mirror(addr)] = val;
  }
}

static void nes_init_nrom(nes_t *nes) {
  if (nes->cart.rom16_count) {
    nes->mem.prg[0] = nes->cart.rom[nes->cart.rom16_count - 1];
    nes->mem.prg[1] = nes->cart.rom[0];
  }
}

static void nes_cleanup_nrom(nes_t *nes) {
  return;
}

// registry stuff

MAPPER_REG_FUNC
static void nes_register_nrom() {
  static const char *mapper_name = "NROM";
  static nes_mapper_funcs_t mapper_funcs = {
    .init = nes_init_nrom, .cleanup = nes_cleanup_nrom,
    .read = nes_mem_read_nrom, .write = nes_mem_write_nrom,
    .vread = nes_vmem_read_nrom, .vwrite = nes_vmem_write_nrom,
  };

  nes_reg_mapper(NES_MAPPER_ID_NROM, mapper_name, &mapper_funcs);
}

MAPPER_UNREG_FUNC
static void nes_unregister_nrom() {
  nes_unreg_mapper(NES_MAPPER_ID_NROM);
}
