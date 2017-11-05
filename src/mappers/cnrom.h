#pragma once

#include "../nes_mappers.h"
#include "../nes_mem.h"
#include "../nes_apu.h"
#include "../nes_input.h"
#include "../nes_ppu.h"

#define NES_MAPPER_ID_CNROM 3

// WARNING: this uses the extra field of nes_mapper_t directly to store
// a size_t value (the bank index)

static uint8_t nes_mem_read_cnrom(nes_t *nes, uint16_t addr) {
  if (addr < 0x2000) return nes_ram_read(nes, addr & 0x07FF);
  if ((addr == 0x4016) || (addr == 0x4017))
    return nes_input_read(nes, addr - 0x4016);
  if (addr < 0x4000) return nes_ppu_read(nes, addr & 0x0007);
  if (addr < 0x4020) return nes_apu_read(nes, addr - 0x4000);
  if (addr >= 0xC000) return nes_prg_read(nes, 0, addr - 0xC000);
  if (addr >= 0x8000) return nes_prg_read(nes, 1, addr - 0x8000);

  return 0x00;
}

static void nes_mem_write_cnrom(nes_t *nes, uint16_t addr, uint8_t val) {
  if (addr < 0x2000) {nes_ram_write(nes, addr & 0x07FF, val); return;}
  if (addr < 0x4000) {nes_ppu_write(nes, addr & 0x0007, val); return;}
  if (addr == 0x4014) {nes_ppu_oamdma(nes, val); return;}
  if ((addr == 0x4016) || (addr == 0x4017)) {
    nes_input_write(nes, addr - 0x4016, val);
    return;
  }
  if (addr < 0x4020) {nes_apu_write(nes, addr - 0x4000, val); return;}
  if (addr >= 0x8000) {
    size_t bank_id = val & 0x03;
    nes->cart.mapper.extra = (void *)(bank_id);
  }
}

static uint8_t nes_vmem_read_cnrom(nes_t *nes, uint16_t addr) {
  addr &= 0x3FFF;
  if (addr >= 0x3F00) {
    addr &= 0x001F;
    if (addr >= 0x10 && (addr & 0x03) == 0)
      addr -= 0x10;
    return nes->vmem.pal[addr];
  }
  if (addr < 0x2000) {
    size_t bank_id = (size_t)(nes->cart.mapper.extra);
    if (nes->cart.vram8_count > bank_id)
      return nes->cart.vram[bank_id][addr & 0x1FFF];
    else
      return 0x00;
  }
  return nes->vmem.vram[nes->cart.mirror(addr)];
}

static void nes_vmem_write_cnrom(nes_t *nes, uint16_t addr, uint8_t val) {
  addr &= 0x3FFF;
  if (addr >= 0x3F00) {
    addr &= 0x001F;
    if (addr >= 0x10 && (addr & 0x03) == 0)
      addr -= 0x10;
    nes->vmem.pal[addr] = val;
  } else if (addr < 0x2000) {
    size_t bank_id = (size_t)(nes->cart.mapper.extra);
    if (nes->cart.chr_ram && nes->cart.vram8_count > bank_id)
      nes->cart.vram[bank_id][addr & 0x1FFF] = val;  
  } else {
    nes->vmem.vram[nes->cart.mirror(addr)] = val;
  }
}

static void nes_init_cnrom(nes_t *nes) {
  nes->cart.mapper.extra = NULL;
  if (nes->cart.rom16_count) {
    nes->mem.prg[0] = nes->cart.rom[nes->cart.rom16_count - 1];
    nes->mem.prg[1] = nes->cart.rom[0];
  }
}

static void nes_cleanup_cnrom(nes_t *nes) {
  nes->cart.mapper.extra = NULL;
}

// registry stuff

MAPPER_REG_FUNC
static void nes_register_cnrom() {
  static const char *mapper_name = "CNROM";
  static nes_mapper_funcs_t mapper_funcs = {
    .init = nes_init_cnrom, .cleanup = nes_cleanup_cnrom,
    .read = nes_mem_read_cnrom, .write = nes_mem_write_cnrom,
    .vread = nes_vmem_read_cnrom, .vwrite = nes_vmem_write_cnrom,
  };

  nes_reg_mapper(NES_MAPPER_ID_CNROM, mapper_name, &mapper_funcs);
}

MAPPER_UNREG_FUNC
static void nes_unregister_cnrom() {
  nes_unreg_mapper(NES_MAPPER_ID_CNROM);
}
