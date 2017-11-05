#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bitops.h"
#include "nes_structs.h"
#include "nes_mem.h"
#include "nes_cpu.h"
#include "nes_ppu.h"

// helper macros for PPU register access

#define PPU_GET_FLAG(f) BITGET(nes->ppu.flags, f)
#define PPU_GET_CTRL(f) BITGET(nes->ppu.ctrl, f)
#define PPU_GET_MASK(f) BITGET(nes->ppu.mask, f)
#define PPU_GET_STATUS(f) BITGET(nes->ppu.status, f)
#define PPU_CLR_FLAG(f) nes->ppu.flags = BITCLR(nes->ppu.flags, f)
#define PPU_CLR_STATUS(f) nes->ppu.status = BITCLR(nes->ppu.status, f)
#define PPU_SET_FLAG(f) nes->ppu.flags = BITSET(nes->ppu.flags, f)
#define PPU_SET_STATUS(f) nes->ppu.status = BITSET(nes->ppu.status, f)
#define PPU_GET_2NDWRITE() BITGET(nes->ppu.flags, NES_PPU_FLAG_OFFSET)
#define PPU_TGL_2NDWRITE() nes->ppu.flags = BITTGL(nes->ppu.flags, NES_PPU_FLAG_OFFSET)
#define PPU_CLR_2NDWRITE() nes->ppu.flags = BITCLR(nes->ppu.flags, NES_PPU_FLAG_OFFSET)

// standard NES palette in ARGB8888
const uint32_t nes_palette[64] = {
  0xFF666666, 0xFF002A88, 0xFF1412A7, 0xFF3B00A4, 0xFF5C007E, 0xFF6E0040, 0xFF6C0600, 0xFF561D00,
  0xFF333500, 0xFF0B4800, 0xFF005200, 0xFF004F08, 0xFF00404D, 0xFF000000, 0xFF000000, 0xFF000000,
  0xFFADADAD, 0xFF155FD9, 0xFF4240FF, 0xFF7527FE, 0xFFA01ACC, 0xFFB71E7B, 0xFFB53120, 0xFF994E00,
  0xFF6B6D00, 0xFF388700, 0xFF0C9300, 0xFF008F32, 0xFF007C8D, 0xFF000000, 0xFF000000, 0xFF000000,
  0xFFFFFEFF, 0xFF64B0FF, 0xFF9290FF, 0xFFC676FF, 0xFFF36AFF, 0xFFFE6ECC, 0xFFFE8170, 0xFFEA9E22,
  0xFFBCBE00, 0xFF88D800, 0xFF5CE430, 0xFF45E082, 0xFF48CDDE, 0xFF4F4F4F, 0xFF000000, 0xFF000000,
  0xFFFFFEFF, 0xFFC0DFFF, 0xFFD3D2FF, 0xFFE8C8FF, 0xFFFBC2FF, 0xFFFEC4EA, 0xFFFECCC5, 0xFFF7D8A5,
  0xFFE4E594, 0xFFCFEF96, 0xFFBDF4AB, 0xFFB3F3CC, 0xFFB5EBF2, 0xFFB8B8B8, 0xFF000000, 0xFF000000
/*
  0xFF656565, 0xFF002D69, 0xFF131F7F, 0xFF3C137C, 0xFF600B62, 0xFF730A37, 0xFF710F07, 0xFF5A1A00,
  0xFF342800, 0xFF0B3400, 0xFF003C00, 0xFF003D10, 0xFF003840, 0xFF010101, 0xFF010101, 0xFF010101,
  0xFFAEAEAE, 0xFF0F63B3, 0xFF4051D0, 0xFF7841CC, 0xFFA736A9, 0xFFC03470, 0xFFBD3C30, 0xFF9F4A00,
  0xFF6D5C00, 0xFF366D00, 0xFF077704, 0xFF00793D, 0xFF00727D, 0xFF010101, 0xFF010101, 0xFF010101,
  0xFFFEFEFF, 0xFF5DB3FF, 0xFF8FA1FF, 0xFFC890FF, 0xFFF785FA, 0xFFFF83C0, 0xFFFF8B7F, 0xFFEF9A49,
  0xFFBDAC2C, 0xFF85BC2F, 0xFF55C753, 0xFF3CC98C, 0xFF3EC2CD, 0xFF4E4E4E, 0xFF010101, 0xFF010101,
  0xFFFEFEFF, 0xFFBCDFFF, 0xFFD1D8FF, 0xFFE8D1FF, 0xFFFBCDFD, 0xFFFFCCE5, 0xFFFFCFCA, 0xFFF8D5B4,
  0xFFE4DCA8, 0xFFCCE3A9, 0xFFB9E8B8, 0xFFAEE8D0, 0xFFAFE5EA, 0xFFB6B6B6, 0xFF010101, 0xFF010101,
*/
};

// this gets called at power on and reset
void nes_ppu_reset(nes_ppu_t *ppu) {
  ppu->flags = BITSET(ppu->flags, NES_PPU_FLAG_RESET);
  ppu->cycle = 340;
  ppu->scanline = 240;
  ppu->frame = 0;
  ppu->ctrl = 0x00;
  ppu->mask = 0x00;
  ppu->oam_addr = 0x00;
}

void nes_ppu_init(nes_ppu_t *ppu) {
  memset(ppu, 0x00, sizeof(nes_ppu_t));
  ppu->front = calloc(1, sizeof(nes_ppu_screen_t));
  ppu->back = calloc(1, sizeof(nes_ppu_screen_t));
  nes_ppu_reset(ppu);
}

// writes a byte to PPU bus
// the value should decay after around 77k ticks, apparently
static inline uint8_t nes_ppu_refresh_bus(nes_t *nes, uint8_t v) {
  nes->ppu.bus_decay = 77777; // TODO: actually make this work
  nes->ppu.bus = v;
  return v;
}

// checks if a NMI should be generated
// if PPUCTRL.NMI is set and last nes_ppu_tick call set the NMI state
// flag, AND one or both of these didn't happen on the previous
// nes_ppu_tick call, then a NMI will be generated after 15 PPU ticks
// this delay helps with some PPU timing sensitive games  
static inline void nes_ppu_nmi_update(nes_t *nes) {
  uint8_t nmi = PPU_GET_CTRL(NES_PPU_CTRL_NMI) &&
    PPU_GET_FLAG(NES_PPU_FLAG_NMI);
  if (nmi && !nes->ppu.nmi_prev)
    nes->ppu.nmi_delay = 15;
  nes->ppu.nmi_prev = nmi;
}

// handles a vblank
// uses double buffering and also sets the NMI and RENDER state flags
static inline void nes_ppu_set_vblank(nes_t *nes) {
  nes_ppu_screen_t *tmp = nes->ppu.back;
  nes->ppu.back = nes->ppu.front;
  nes->ppu.front = tmp;
  PPU_SET_FLAG(NES_PPU_FLAG_NMI);
  PPU_SET_FLAG(NES_PPU_FLAG_RENDER);
  nes_ppu_nmi_update(nes);
}

// clears the NMI flag
static inline void nes_ppu_clr_vblank(nes_t *nes) {
  PPU_CLR_FLAG(NES_PPU_FLAG_NMI);
  nes_ppu_nmi_update(nes);
}

// memory write function for PPU address space ($2000-$2007)
// addr is actually the register index (0-7), not the actual address
void nes_ppu_write(nes_t *nes, uint16_t addr, uint8_t val) {
  // store last written byte on the bus
  nes_ppu_refresh_bus(nes, val);
  switch (addr) {
    case 0: // $2000 - PPUCTRL
      // store control flags
      nes->ppu.ctrl = val;
      // start NMI timer if necessary
      nes_ppu_nmi_update(nes);
      // store nametable index in bits 10-11 of shadow address register
      nes->ppu.tmp_addr = (nes->ppu.tmp_addr & 0xF3FF) | ((val & 0x03) << 10);
      break;
    case 1: // $2001 - PPUMASK
      nes->ppu.mask = val;
      break;
    case 2: // $2002 - PPUSTATUS
      // is read-only
      break;
    case 3: // $2003 - OAMADDR
      nes->ppu.oam_addr = val;
      break;
    case 4: // $2004 - OAMDATA
      nes->vmem.oam[nes->ppu.oam_addr++] = val;
      break;
    case 5: // $2005 - PPUSCROLL
      if (!PPU_GET_2NDWRITE()) {
        // bits 0-4: coarse X scroll
        nes->ppu.tmp_addr = (nes->ppu.tmp_addr & 0xFFE0) | (val >> 3);
        // store fine X scroll in separate register
        nes->ppu.fine_x = val & 0x07;
      } else {
        // bits 12-14: fine Y scroll
        nes->ppu.tmp_addr = (nes->ppu.tmp_addr & 0x8FFF) | ((val & 0x07) << 12);
        // bits 5-9: coarse Y scroll
        nes->ppu.tmp_addr = (nes->ppu.tmp_addr & 0xFC1F) | ((val & 0xF8) << 2);
      }
      PPU_TGL_2NDWRITE();
      break;
    case 6: // $2006 - PPUADDR
      if (!PPU_GET_2NDWRITE()) {
        // store high byte of address in shadow address register
        nes->ppu.tmp_addr = (nes->ppu.tmp_addr & 0x80FF) | ((val & 0x3F) << 8);
      } else {
        // store low byte of address in shadow address register
        nes->ppu.tmp_addr = (nes->ppu.tmp_addr & 0xFF00) | val;
        // address fully received: copy shadow register to address register
        nes->ppu.vmem_addr = nes->ppu.tmp_addr;
      }
      PPU_TGL_2NDWRITE();
      break;
    case 7: // $2007 - PPUDATA
      // write byte to video memory and increment address
      nes_vmem_writeb(nes, nes->ppu.vmem_addr, val);
      nes->ppu.vmem_addr += (PPU_GET_CTRL(NES_PPU_CTRL_ADDRINC)) ? 32 : 1;
      break;
    default:
      fprintf(stderr, "Invalid PPU register %d (wrote 0x%02X)\n", addr, val);
  }
}

// memory read function for PPU address space ($2000-$2007)
// addr is actually the register index (0-7), not the actual address
uint8_t nes_ppu_read(nes_t *nes, uint16_t addr) {
  uint8_t res = nes->ppu.bus; // return bus value if nothing changed
  uint8_t tmp = 0x00;
  switch (addr) {
    case 0: // $2000 - PPUCTRL
      // is write-only
      break;
    case 1: // $2001 - PPUMASK
      // is write-only
      break;
    case 2: // $2002 - PPUSTATUS
      // low 4 bits of status are copied from last write
      res = nes->ppu.status | (nes->ppu.bus & 0x1F);
      // set VBLANK if it has happened
      if (PPU_GET_FLAG(NES_PPU_FLAG_NMI))
        res = BITSET(res, NES_PPU_STATUS_VBLANK);
      // reading PPUSTATUS clears VBLANK, NMI and second write flags
      PPU_CLR_FLAG(NES_PPU_FLAG_NMI);
      nes_ppu_nmi_update(nes);
      PPU_CLR_2NDWRITE();
      break;
    case 3: // $2003 - OAMADDR
      // is write-only
      break;
    case 4: // $2004 - OAMDATA
      res = nes->vmem.oam[nes->ppu.oam_addr];
      break;
    case 5: // $2005 - PPUSCROLL
      // is write-only
      break;
    case 6: // $2006 - PPUADDR
      // is write-only
      break;
    case 7: // $2007 - PPUDATA
      res = nes_vmem_readb(nes, nes->ppu.vmem_addr);
      if((nes->ppu.vmem_addr & 0x3FFF) < 0x3F00) {
        tmp = nes->ppu.readb;
        nes->ppu.readb = res;
        res = tmp;
      } else {
        nes->ppu.readb = nes_vmem_readb(nes, nes->ppu.vmem_addr - 0x1000);
      }
      nes->ppu.vmem_addr += (PPU_GET_CTRL(NES_PPU_CTRL_ADDRINC)) ? 32 : 1;
      break;
    default:
      fprintf(stderr, "Invalid PPU register %d (read)\n", addr);
      return 0x00;
  }
  return res;
}

// handles OAM DMA ($4014 writes)
// stalls the CPU for 513 or 514 cycles
void nes_ppu_oamdma(nes_t *nes, uint8_t page) {
  uint16_t addr = page * 0x100;
  for (uint16_t i = 0; i < 256; ++i) {
    nes->vmem.oam[nes->ppu.oam_addr] = nes_mem_readb(nes, addr);
    nes->ppu.oam_addr++;
    addr++;
  }
  nes->cpu.stall += 513 + (nes->cpu.cycle & 0x01);
}

// rendering logic

// returns ARGB8888 value from NES color index, handling color emphasis bits
static inline uint32_t nes_ppu_get_color(nes_t *nes, uint8_t col) {
  // TODO: precompute all of this or at least the dark palette
  if (PPU_GET_MASK(NES_PPU_MASK_GRAYSCALE)) col &= 0x30;
  uint32_t ret = nes_palette[col & 0x3F];
  if ((nes->ppu.mask & 0xE0) == 0xE0) {
    // if all three emphasis bits are set, just darken the color
    uint8_t r = (ret & 0x00FF0000) >> 17;
    uint8_t g = (ret & 0x0000FF00) >> 9;
    uint8_t b = (ret & 0x000000FF) >> 1;
    ret = 0xFF000000 | (r << 16) | (g << 8) | b;
  } else if (ret & 0x00FEFEFE) {
    // otherwise maximize particular color component
    if (PPU_GET_MASK(NES_PPU_MASK_ERED)) ret |= 0x00FF0000;
    if (PPU_GET_MASK(NES_PPU_MASK_EGREEN)) ret |= 0x0000FF00;
    if (PPU_GET_MASK(NES_PPU_MASK_EBLUE)) ret |= 0x000000FF;
  }
  return ret;
}

// advances vram pointer to the next tile row
static void nes_ppu_increment_y(nes_t *nes) {
  if ((nes->ppu.vmem_addr & 0x7000) != 0x7000) {
    nes->ppu.vmem_addr += 0x1000;
  } else {
    nes->ppu.vmem_addr &= 0x8FFF;
    uint16_t y = (nes->ppu.vmem_addr & 0x03E0) >> 5;
    if (y == 29) {
      y = 0;
      nes->ppu.vmem_addr ^= 0x0800;
    } else if (y == 31) {
      y = 0;
    } else {
      y++;
    }
    nes->ppu.vmem_addr = (nes->ppu.vmem_addr & 0xFC1F) | (y << 5);
  }
}

// advances vram pointer to the next tile column
static void nes_ppu_increment_x(nes_t *nes) {
  if ((nes->ppu.vmem_addr & 0x001F) == 0x1F) {
    nes->ppu.vmem_addr &= 0xFFE0;
    nes->ppu.vmem_addr ^= 0x0400;
  } else {
    nes->ppu.vmem_addr++;
  }
}

// copies Y part of the temp pointer to the actual vram pointer
static inline void nes_ppu_copy_y(nes_t *nes) {
  nes->ppu.vmem_addr =
    (nes->ppu.vmem_addr & 0x841F) | (nes->ppu.tmp_addr & 0x7BE0);
}

// copies X part of the temp pointer to the actual vram pointer
static inline void nes_ppu_copy_x(nes_t *nes) {
  nes->ppu.vmem_addr =
    (nes->ppu.vmem_addr & 0xFBE0) | (nes->ppu.tmp_addr & 0x041F);
}

// fetches nametable data for current tile
static inline void nes_ppu_fetch_nta(nes_t *nes) {
  uint16_t t = nes->ppu.vmem_addr;
  nes->ppu.tile.nta = nes_vmem_readb(nes, 0x2000 | (t & 0x0FFF));
}

// fetches attribute data for current tile
static inline void nes_ppu_fetch_attr(nes_t *nes) {
  uint16_t t = nes->ppu.vmem_addr;
  uint16_t addr = 0x23C0 | (t & 0x0C00) | ((t >> 4) & 0x38) | ((t >> 2) & 0x07);
  uint16_t shift = ((t >> 4) & 0x04) | (t & 0x02);
  nes->ppu.tile.attr = ((nes_vmem_readb(nes, addr) >> shift) & 0x03) << 2;
}

// fetches tile graphics for current tile
static inline void nes_ppu_fetch_tile(nes_t *nes, int hi) {
  uint8_t fine_y = (nes->ppu.vmem_addr >> 12) & 0x07;
  uint8_t table = !!PPU_GET_CTRL(NES_PPU_CTRL_BGTABLE);
  uint8_t tile = nes->ppu.tile.nta;
  uint16_t addr = 0x1000 * table + tile * 16 + fine_y;
  if (hi)
    nes->ppu.tile.data_hi = nes_vmem_readb(nes, addr + 0x08);
  else
    nes->ppu.tile.data_lo = nes_vmem_readb(nes, addr);
}

// forms complete tile data from attributes and hi and lo bytes
static inline void nes_ppu_store_tile(nes_t *nes) {
  uint32_t data = 0;
  for (int i = 0; i < 8; ++i) {
    uint8_t a = nes->ppu.tile.attr;
    uint8_t p1 = (nes->ppu.tile.data_lo & 0x80) >> 7;
    uint8_t p2 = (nes->ppu.tile.data_hi & 0x80) >> 6;
    nes->ppu.tile.data_lo <<= 1;
    nes->ppu.tile.data_hi <<= 1;
    data <<= 4;
    data |= a | p1 | p2;
  }
  nes->ppu.tile.data |= data;
}

// returns sprite data for the row-th row of the i-th sprite
static inline uint64_t nes_ppu_fetch_spr(nes_t *nes, int i, int row) {
  uint8_t tile = nes->vmem.oam[i * 4 + 1];
  uint8_t attr = nes->vmem.oam[i * 4 + 2];

  uint8_t table = 0;
  if (PPU_GET_CTRL(NES_PPU_CTRL_SPRSIZE)) {
    if (attr & 0x80) row = 15 - row;
    table = tile & 0x01;
    tile &= 0xFE;
    if (row > 7) {tile++; row -= 8;}
  } else {
    if (attr & 0x80) row = 7 - row;
    table = !!PPU_GET_CTRL(NES_PPU_CTRL_SPRTABLE);
  }
  uint16_t addr = 0x1000 * table + tile * 16 + row;

  uint8_t a = (attr & 0x03) << 2;
  uint8_t lo = nes_vmem_readb(nes, addr);
  uint8_t hi = nes_vmem_readb(nes, addr + 0x08);

  uint32_t data = 0;
  uint8_t p1, p2;
  if (attr & 0x40) {
    for (int i = 0; i < 8; ++i) {
      p1 = (lo & 0x01) << 0;
      p2 = (hi & 0x01) << 1;
      lo >>= 1;
      hi >>= 1;
      data <<= 4;
      data |= a | p1 | p2;
    }
  } else {
    for (int i = 0; i < 8; ++i) {
      p1 = (lo & 0x80) >> 7;
      p2 = (hi & 0x80) >> 6;
      lo <<= 1;
      hi <<= 1;
      data <<= 4;
      data |= a | p1 | p2;
    }
  }

  return data;
}

// prepares sprite data (fills the nes_ppu_spr_t structs)
static inline void nes_ppu_process_sprites(nes_t *nes) {
  int h = (PPU_GET_CTRL(NES_PPU_CTRL_SPRSIZE)) ? 16 : 8;
  int n = 0;
  for (uint32_t i = 0; i < 64; ++i) {
    uint8_t y = nes->vmem.oam[i * 4 + 0];
    uint8_t a = nes->vmem.oam[i * 4 + 2];
    uint8_t x = nes->vmem.oam[i * 4 + 3];
    int row = nes->ppu.scanline - y;
    if (row < 0 || row >= h) continue;
    if (n < 8) {
      nes->ppu.spr[n].data = nes_ppu_fetch_spr(nes, i, row);
      nes->ppu.spr[n].pos = x;
      nes->ppu.spr[n].pri = (a >> 5) & 0x01;
      nes->ppu.spr[n].idx = i;
    }
    n++;
  }
  if (n > 8) {
    n = 8;
    PPU_SET_STATUS(NES_PPU_STATUS_OVERFLOW);
  }
  nes->ppu.spr_count = n;
}

// returns background color for current pixel
static inline uint8_t nes_ppu_get_bg_pixel(nes_t *nes) {
  if (!PPU_GET_MASK(NES_PPU_MASK_BG)) return 0x00;
  uint32_t data = (nes->ppu.tile.data >> 32) >> ((7 - nes->ppu.fine_x) * 4);
  return (uint8_t)(data & 0x0F);
}

// returns sprite color and index for current pixel (both 0 if no sprite here)
static inline void nes_ppu_get_spr_pixel(nes_t *nes, uint8_t *p, uint8_t *n) {
  *p = 0x00; *n = 0x00;
  if (!PPU_GET_MASK(NES_PPU_MASK_SPR)) return;
  for (uint32_t i = 0; i < nes->ppu.spr_count; ++i) {
    int offset = nes->ppu.cycle - 1 - nes->ppu.spr[i].pos;
    if (offset < 0 || offset > 7) continue;
    offset = 7 - offset;
    uint8_t col = (nes->ppu.spr[i].data >> (offset * 4)) & 0x0F;
    if (!(col & 0x03)) continue;
    *p = col; *n = i;
    return;
  }
}

// draws current pixel into the back buffer
static inline void nes_ppu_render_pixel(nes_t *nes) {
  int x = nes->ppu.cycle - 1;
  int y = nes->ppu.scanline;

  uint8_t bg = nes_ppu_get_bg_pixel(nes);
  uint8_t spr, spr_idx;
  nes_ppu_get_spr_pixel(nes, &spr, &spr_idx);

  if (x < 8 && !PPU_GET_MASK(NES_PPU_MASK_LEFTBG)) bg = 0;
  if (x < 8 && !PPU_GET_MASK(NES_PPU_MASK_LEFTSPR)) spr = 0;

  uint8_t b = (bg & 0x03);
  uint8_t s = (spr & 0x03);

  uint8_t col = 0x00;
  if (b && !s) col = bg;
  else if (!b && s) col = spr | 0x10;
  else if (b && s) {
    if (nes->ppu.spr[spr_idx].idx == 0 && x < 255)
      PPU_SET_STATUS(NES_PPU_STATUS_SPRITE0);
    if (nes->ppu.spr[spr_idx].pri == 0)
      col = spr | 0x10;
    else
      col = bg;
  }

  nes->ppu.back->data[y][x] = nes_ppu_get_color(nes, nes->vmem.pal[col]);
}

// counts all kinds of shit: increments cycle, scanline and frame counters,
// decrements NMI delay and triggers NMI if possible
static void nes_ppu_clock(nes_t *nes) {
  if (nes->ppu.nmi_delay > 0) {
    nes->ppu.nmi_delay--;
    if (nes->ppu.nmi_delay == 0 &&
        PPU_GET_CTRL(NES_PPU_CTRL_NMI) &&
        PPU_GET_FLAG(NES_PPU_FLAG_NMI))
      nes_cpu_nmi(nes);
  }

  if (PPU_GET_MASK(NES_PPU_MASK_BG) || PPU_GET_MASK(NES_PPU_MASK_SPR)) {
    if (nes->ppu.frame_end && nes->ppu.scanline == 261 && nes->ppu.cycle == 339) {
      nes->ppu.cycle = 0;
      nes->ppu.scanline = 0;
      nes->ppu.frame++;
      nes->ppu.frame_end ^= 1;
      return;
    }
  }

  nes->ppu.cycle++;
  if (nes->ppu.cycle > 340) {
    nes->ppu.cycle = 0;
    nes->ppu.scanline++;
    if (nes->ppu.scanline > 261) {
      nes->ppu.scanline = 0;
      nes->ppu.frame++;
      nes->ppu.frame_end ^= 1;
    }
  }
}

void nes_ppu_tick(nes_t *nes) {
  nes_ppu_clock(nes);

  int render =
    PPU_GET_MASK(NES_PPU_MASK_BG) || PPU_GET_MASK(NES_PPU_MASK_SPR);
  int pre_line = nes->ppu.scanline == 261;
  int vis_line = nes->ppu.scanline < 240;
  int render_line = pre_line || vis_line;

  int pre_cycle = nes->ppu.cycle >= 321 && nes->ppu.cycle <= 336;
  int vis_cycle = nes->ppu.cycle >= 1 && nes->ppu.cycle <= 256;
  int fetch_cycle = pre_cycle || vis_cycle;

  if (render) {
    if (vis_line && vis_cycle)
      nes_ppu_render_pixel(nes);

    if (render_line && fetch_cycle) {
      nes->ppu.tile.data <<= 4;
      switch (nes->ppu.cycle & 0x07) {
        case 1: nes_ppu_fetch_nta(nes); break;
        case 3: nes_ppu_fetch_attr(nes); break;
        case 5: nes_ppu_fetch_tile(nes, 0); break;
        case 7: nes_ppu_fetch_tile(nes, 1); break;
        case 0: nes_ppu_store_tile(nes); break;
      }
    }

    if (pre_line && nes->ppu.cycle >= 280 && nes->ppu.cycle <= 304)
      nes_ppu_copy_y(nes);

    if (render_line) {
      if (fetch_cycle && (nes->ppu.cycle  & 0x07) == 0)
        nes_ppu_increment_x(nes);
      if (nes->ppu.cycle == 256)
        nes_ppu_increment_y(nes);
      if (nes->ppu.cycle == 257)
        nes_ppu_copy_x(nes);
    }

    if (nes->ppu.cycle == 257) {
      if (vis_line)
        nes_ppu_process_sprites(nes);
      else
        nes->ppu.spr_count = 0;
    }
  } else if (vis_line && vis_cycle) {
    nes->ppu.back->data[nes->ppu.scanline][nes->ppu.cycle - 1] =
      nes_ppu_get_color(nes, nes->vmem.pal[0x00]);
  }

  if (nes->ppu.cycle == 1) {
    if (nes->ppu.scanline == 241)
      nes_ppu_set_vblank(nes);
    if (pre_line) {
      nes_ppu_clr_vblank(nes);
      PPU_CLR_STATUS(NES_PPU_STATUS_SPRITE0);
      PPU_CLR_STATUS(NES_PPU_STATUS_OVERFLOW);
    }
  }
}

void nes_ppu_cleanup(nes_ppu_t *ppu) {
  if (ppu->back) free(ppu->back);
  if (ppu->front) free(ppu->front);
}
