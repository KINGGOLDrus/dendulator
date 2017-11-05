#pragma once

// NOTE: all of these enums are bit indices, not masks

// PPU internal state flags
enum nes_ppu_flags {
  NES_PPU_FLAG_RESET,  // 1 when PPU is resetting/powering on
  NES_PPU_FLAG_OFFSET, // 1 when PPU is waiting for the second data write
  NES_PPU_FLAG_RENDER, // 1 when a frame is ready to be displayed
  NES_PPU_FLAG_NMI,    // set after NMI was requested by the tick function
};

// PPUCTRL ($2000) register bits
enum nes_ppu_ctrl {
  NES_PPU_CTRL_ADDRINC  = 2, // if 1, address register increments by 32
  NES_PPU_CTRL_SPRTABLE = 3, // sprites nametable index (0-1)
  NES_PPU_CTRL_BGTABLE  = 4, // background tiles nametable index (0-1)
  NES_PPU_CTRL_SPRSIZE  = 5, // if 1, sprites are 8x16
  NES_PPU_CTRL_MASTER   = 6, // master/slave switch (unused)
  NES_PPU_CTRL_NMI      = 7, // if 1, NMIs are allowed
};

// PPUMASK ($2001) register bits
enum nes_ppu_mask {
  NES_PPU_MASK_GRAYSCALE, // if 1, grayscale palette
  NES_PPU_MASK_LEFTBG,    // if 0, doesn't draw the leftmost 8 px of bg tiles
  NES_PPU_MASK_LEFTSPR,   // if 0, doesn't draw the leftmost 8 px of sprites
  NES_PPU_MASK_BG,        // if 0, bg tiles are not displayed
  NES_PPU_MASK_SPR,       // if 0, sprites are not displayed
  NES_PPU_MASK_ERED,      // red emphasis bit
  NES_PPU_MASK_EGREEN,    // green emphasis bit
  NES_PPU_MASK_EBLUE,     // blue emphasis bit
};

// PPUSTATUS ($2002) register bits
enum nes_ppu_status {
  NES_PPU_STATUS_OVERFLOW = 5, // set when more than 8 sprites on current line
  NES_PPU_STATUS_SPRITE0  = 6, // set if sprite0 was just hit
  NES_PPU_STATUS_VBLANK   = 7, // set during vblank
};

// standard NES palette in ARGB8888
extern const uint32_t nes_palette[64];

void nes_ppu_init(nes_ppu_t *ppu);
void nes_ppu_reset(nes_ppu_t *ppu);
void nes_ppu_cleanup(nes_ppu_t *ppu);
void nes_ppu_tick(nes_t *nes);
void nes_ppu_write(nes_t *nes, uint16_t addr, uint8_t val);
void nes_ppu_oamdma(nes_t *nes, uint8_t addr);
uint8_t nes_ppu_read(nes_t *nes, uint16_t addr);
