#pragma once

#include "../nes_mappers.h"
#include "../nes_mem.h"
#include "../nes_apu.h"
#include "../nes_input.h"
#include "../nes_ppu.h"
#include "../nes_cart.h"
#include "../error.h"
#include "../errcodes.h"

#define NES_MAPPER_ID_MMC1 1

#define MMC1_R0_MIRROR  0x01 // screen mirroring bit mask
#define MMC1_R0_ONESCR  0x02 // single-screen mirroring bit mask
#define MMC1_R0_PRGAREA 0x04 // PRG bank select bit mask
#define MMC1_R0_PRGSIZE 0x08 // PRG bank size select bit mask
#define MMC1_R0_VROMSW  0x10 // CHR mode bit mask
#define MMC1_R0_RESET   0x80 // reset bit mask

#define MMC1_R1_VROMB1  0x0F // CHR bank select bits mask
#define MMC1_R1_256KSEL 0x10 // CHR bank size select bit mask
#define MMC1_R1_RESET   0x80 // reset bit mask

#define MMC1_R2_VROMB2  0x0F // CHR bank select bits mask
#define MMC1_R2_256KSEL 0x10 // CHR bank size select bit mask
#define MMC1_R2_RESET   0x80 // reset bit mask

#define MMC1_R3_VROMB2  0x0F // PRG bank number bits mask
#define MMC1_R3_SAVECE  0x10 // PRG-RAM enable bit mask
#define MMC1_R3_RESET   0x80 // reset bit mask

#define MMC1_R0_DEF MMC1_R0_PRGSIZE | MMC1_R0_PRGAREA // initial control
                                                      // register value
#define MMC1_R1_DEF 0 // initial CHR0 register value
#define MMC1_R2_DEF 0 // initial CHR1 register value
#define MMC1_R3_DEF 0 // initial PRG register value

// extra mapper data for MMC1
typedef struct {
  uint8_t r0; // control register value
  uint8_t r1; // CHR0 register value
  uint8_t r2; // CHR1 register value
  uint8_t r3; // PRG register value

  uint8_t cur_bank; // currently selected bank
  uint8_t old_switch_area; // last PRG bank switch value

  uint32_t chr_bank[2]; // CHR bank numbers
  uint32_t chr_bank_sw; // last CHR bank switch value

  uint8_t bit; // current bit number
  uint8_t bit_buf; // bit buffer
} nes_mmc1_extra_t;

#define MMC1_GETBIT(v) BITMGET(v, 0x01)
#define MMC1_ADDBIT(a, v, b) BITCHG(a, b, v)

static uint8_t nes_mem_read_mmc1(nes_t *nes, uint16_t addr) {
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

// applies MMC1 settings, changing mirroring and banks
static inline void nes_mmc1_apply(nes_t *nes) {
  nes_mmc1_extra_t *ex = nes->cart.mapper.extra;

  switch (ex->r0 & 0x03) {
    case 0: nes_cart_set_mirroring(nes, MIRROR_SINGLESCREEN0); break;
    case 1: nes_cart_set_mirroring(nes, MIRROR_SINGLESCREEN1); break;
    case 2: nes_cart_set_mirroring(nes, MIRROR_VERTICAL); break;
    case 3: nes_cart_set_mirroring(nes, MIRROR_HORIZONTAL); break;
  }

  if ((ex->old_switch_area != (ex->r0 & MMC1_R0_PRGAREA)) &&
      (ex->r0 & MMC1_R0_PRGSIZE)) {
    if (ex->r0 & MMC1_R0_PRGAREA) {
      nes->mem.prg[1] = nes->cart.rom[ex->cur_bank];
      nes->mem.prg[0] = nes->cart.rom[nes->cart.rom16_count - 1];
    } else {
      nes->mem.prg[1] = nes->cart.rom[0];
      nes->mem.prg[0] = nes->cart.rom[ex->cur_bank];
    }
  }

  ex->old_switch_area = ex->r0 & MMC1_R0_PRGAREA;
}

// writes to the R0 MMC1 register
static inline void nes_mmc1_write_r0(nes_t *nes, uint8_t val) {
  nes_mmc1_extra_t *ex = nes->cart.mapper.extra;

  if (val & MMC1_R0_RESET) {
    ex->r0 = MMC1_R0_DEF;
    nes_mmc1_apply(nes);
  } else {
    ex->bit_buf = MMC1_ADDBIT(ex->bit_buf, MMC1_GETBIT(val), ex->bit);
    if (ex->bit < 4) {
      ex->bit++;
    } else {
      ex->bit = 0;
      ex->r0 = ex->bit_buf;
      nes_mmc1_apply(nes);
    }
  }
}

// writes to the R1 MMC1 register
static inline void nes_mmc1_write_r1(nes_t *nes, uint8_t val) {
  nes_mmc1_extra_t *ex = nes->cart.mapper.extra;

  if (val & MMC1_R1_RESET) {
    ex->r1 = MMC1_R1_DEF;
  } else {
    ex->bit_buf = MMC1_ADDBIT(ex->bit_buf, MMC1_GETBIT(val), ex->bit);
    if (ex->bit < 4) {
      ex->bit++;
    } else {
      ex->bit = 0;
      ex->r1 = ex->bit_buf;

      ex->chr_bank_sw = ex->r1;

      if (ex->r0 & MMC1_R0_VROMSW)
        ex->chr_bank[0] = ex->chr_bank_sw;
      else
        ex->chr_bank[0] = ex->chr_bank_sw >> 1;
    }
  }
}

// writes to the R2 MMC1 register
static inline void nes_mmc1_write_r2(nes_t *nes, uint8_t val) {
  nes_mmc1_extra_t *ex = nes->cart.mapper.extra;

  if (val & MMC1_R2_RESET) {
    ex->r2 = MMC1_R2_DEF;
  } else {
    ex->bit_buf = MMC1_ADDBIT(ex->bit_buf, MMC1_GETBIT(val), ex->bit);
    if (ex->bit < 4) {
      ex->bit++;
    } else {
      ex->bit = 0;
      ex->r2 = ex->bit_buf;

      ex->chr_bank_sw = ex->r2;

      if (ex->r0 & MMC1_R0_VROMSW)
        ex->chr_bank[1] = ex->chr_bank_sw;
      else
        ex->chr_bank[1] = ex->chr_bank_sw >> 1;
    }
  }
}

// writes to the R3 MMC1 register
static inline void nes_mmc1_write_r3(nes_t *nes, uint8_t val) {
  nes_mmc1_extra_t *ex = nes->cart.mapper.extra;

  if (val & MMC1_R3_RESET) {
    ex->r3 = MMC1_R3_DEF;
  } else {
    ex->bit_buf = MMC1_ADDBIT(ex->bit_buf, MMC1_GETBIT(val), ex->bit);
    if (ex->bit < 4) {
      ex->bit++;
    } else {
      ex->bit = 0;
      ex->r3 = ex->bit_buf;

      if (ex->r3 >= nes->cart.rom16_count)
        return;

      if (ex->r0 & MMC1_R0_PRGSIZE) {
        if (ex->r0 & MMC1_R0_PRGAREA) {
          nes->mem.prg[1] = nes->cart.rom[ex->r3];
        } else {
          nes->mem.prg[0] = nes->cart.rom[ex->r3];
        }
      } else {
        nes->mem.prg[1] = nes->cart.rom[ex->r3 >> 1];
        nes->mem.prg[0] = nes->cart.rom[(ex->r3 >> 1) + 1];
      }

      if (ex->r3 & MMC1_R3_SAVECE)
        return; // nes_sram_unmap(nes);
      else
        return; // nes_sram_map(nes);
    }
  }
}

// writes to the MMC1 register at specified address
static inline void nes_mmc1_write_reg(nes_t *nes, uint16_t addr, uint8_t val) {
  if (addr <= 0x9FFF) {nes_mmc1_write_r0(nes, val); return;}
  if (addr <= 0xBFFF) {nes_mmc1_write_r1(nes, val); return;}
  if (addr <= 0xDFFF) {nes_mmc1_write_r2(nes, val); return;}
  if (addr <= 0xFFFF) {nes_mmc1_write_r3(nes, val); return;}
}

static void nes_mem_write_mmc1(nes_t *nes, uint16_t addr, uint8_t val) {
  if (addr < 0x2000) {nes_ram_write(nes, addr & 0x07FF, val); return;}
  if (addr < 0x4000) {nes_ppu_write(nes, addr & 0x0007, val); return;}
  if (addr == 0x4014) {nes_ppu_oamdma(nes, val); return;}
  if (addr == 0x4016) {
    nes_input_write(nes, addr - 0x4016, val);
    return;
  }
  if (addr < 0x4020) {nes_apu_write(nes, addr - 0x4000, val); return;}
  if (addr >= 0x8000) {nes_mmc1_write_reg(nes, addr, val); return;}
  if (addr >= 0x6000) {nes_prgram_write(nes, addr - 0x6000, val); return;}
}

static uint8_t nes_vmem_read_mmc1(nes_t *nes, uint16_t addr) {
  nes_mmc1_extra_t *ex = nes->cart.mapper.extra;

  addr &= 0x3FFF;
  if (addr >= 0x3F00) {
    addr &= 0x001F;
    if (addr >= 0x10 && (addr & 0x03) == 0)
      addr -= 0x10;
    return nes->vmem.pal[addr];
  }
  if (addr < 0x1000) {
    if (nes->cart.vram8_count) {
      uint8_t bank = (nes->cart.chr_ram) ? 0 : ex->chr_bank[0];
      uint16_t offset = (bank % 2) * 0x1000;

      return nes->cart.vram[bank / 2][(addr & 0x0FFF) + offset];
    } else
      return 0x00;
  }
  if (addr < 0x2000) {
    if (nes->cart.vram8_count) {
      uint8_t bank = (nes->cart.chr_ram) ? 1 : ex->chr_bank[1];
      uint16_t offset = (bank % 2) * 0x1000;

      return nes->cart.vram[bank / 2][(addr & 0x0FFF) + offset];
    } else
      return 0x00;
  }
  return nes->vmem.vram[nes->cart.mirror(addr)];
}

static void nes_vmem_write_mmc1(nes_t *nes, uint16_t addr, uint8_t val) {
  addr &= 0x3FFF;
  if (addr >= 0x3F00) {
    addr &= 0x001F;
    if (addr >= 0x10 && (addr & 0x03) == 0)
      addr -= 0x10;
    nes->vmem.pal[addr] = val;
  } else if (addr < 0x2000) {
    if (nes->cart.vram8_count && nes->cart.chr_ram)
      nes->cart.vram[0][addr & 0x1FFF] = val;
  } else {
    nes->vmem.vram[nes->cart.mirror(addr)] = val;
  }
}

static void nes_init_mmc1(nes_t *nes) {
  nes_mmc1_extra_t *ex;

  if (!(ex = calloc(1, sizeof(nes_mmc1_extra_t)))) {
    error_set_code(ERR_ROM_INIT);
    error_log_write("Out of memory on extra data allocation!\n");
    return;
  }

  ex->r0 = MMC1_R0_DEF;
  ex->r1 = MMC1_R1_DEF;
  ex->r2 = MMC1_R2_DEF;
  ex->r3 = MMC1_R3_DEF;
  ex->cur_bank = 0;
  ex->old_switch_area = MMC1_R0_PRGAREA;

  ex->bit = 0;
  ex->bit_buf = 0;

  ex->chr_bank[0] = 0;
  ex->chr_bank[1] = 1;

  nes->cart.mapper.extra = ex;

  if (nes->cart.rom16_count) {
    nes->mem.prg[0] = nes->cart.rom[nes->cart.rom16_count - 1];
    nes->mem.prg[1] = nes->cart.rom[0];
  }
}

static void nes_cleanup_mmc1(nes_t *nes) {
  free(nes->cart.mapper.extra);
  nes->cart.mapper.extra = NULL;
}

// registry stuff

MAPPER_REG_FUNC
static void nes_register_mmc1() {
  static const char *mapper_name = "MMC1";
  static nes_mapper_funcs_t mapper_funcs = {
    .init = nes_init_mmc1, .cleanup = nes_cleanup_mmc1,
    .read = nes_mem_read_mmc1, .write = nes_mem_write_mmc1,
    .vread = nes_vmem_read_mmc1, .vwrite = nes_vmem_write_mmc1,
  };

  nes_reg_mapper(NES_MAPPER_ID_MMC1, mapper_name, &mapper_funcs);
}

MAPPER_UNREG_FUNC
static void nes_unregister_mmc1() {
  nes_unreg_mapper(NES_MAPPER_ID_MMC1);
}
