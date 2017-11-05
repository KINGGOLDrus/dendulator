#pragma once

#include <string.h>

#include "nes_structs.h"

// CPU RAM read
static inline uint8_t nes_ram_read(nes_t *nes, uint16_t addr) {
  return nes->mem.ram[addr];
}

// PRG-ROM read
static inline uint8_t nes_prg_read(nes_t *nes, uint8_t bank, uint16_t addr) {
  if (nes->mem.prg[bank] == NULL) return 0x00;

  return nes->mem.prg[bank][addr];
}

// PRG-RAM read
static inline uint8_t nes_prgram_read(nes_t *nes, uint16_t addr) {
  return nes->mem.prgram[addr];
}

// CPU RAM write
static inline void nes_ram_write(nes_t *nes, uint16_t addr, uint8_t val) {
  nes->mem.ram[addr] = val;
}

// PRG-RAM write
static inline void nes_prgram_write(nes_t *nes, uint16_t addr, uint8_t val) {
  nes->mem.prgram[addr] = val;
}

// initializes RAM on power up
static inline void nes_mem_init(nes_mem_t *mem) {
  for (int i = 0; i < 0x800; ++i) mem->ram[i] = (i & 0x04) ? 0xFF : 0x00;
  for (int i = 0; i < 0x2000; ++i) mem->prgram[i] = 0x00;

  mem->prg[0] = NULL;
  mem->prg[1] = NULL;
}

// initializes VRAM on power up
static inline void nes_vmem_init(nes_vmem_t *vmem) {
  for (int i = 0; i < 0x1000; ++i) vmem->vram[i] = 0x00;
  for (int i = 0; i < 0x100; ++i) vmem->oam[i] = 0x00;
}

// reads a byte from PPU address space
static inline uint8_t nes_vmem_readb(nes_t *nes, uint16_t addr) {
  return nes->cart.mapper.funcs.vread(nes, addr);
}

// writes a byte to PPU address space
static inline void nes_vmem_writeb(nes_t *nes, uint16_t addr, uint8_t val) {
  nes->cart.mapper.funcs.vwrite(nes, addr, val);
}

// reads a byte CPU address space
static inline uint8_t nes_mem_readb(nes_t *nes, uint16_t addr) {
  return nes->cart.mapper.funcs.read(nes, addr);
}

// reads a byte from CPU address space using zero-page addressing
static inline uint16_t nes_mem_readb_zp(nes_t *nes, uint16_t addr) {
  return nes_mem_readb(nes, addr & 0xFF);
}

// reads a word from CPU address space
static inline uint16_t nes_mem_readw(nes_t *nes, uint16_t addr) {
  return ((uint16_t)nes_mem_readb(nes, addr + 1) << 8) |
    nes_mem_readb(nes, addr);
}

// reads a word from CPU address space using zero-page addressing
static inline uint16_t nes_mem_readw_zp(nes_t *nes, uint16_t addr) {
  return ((uint16_t)nes_mem_readb(nes, (addr + 1) & 0xFF) << 8) |
    nes_mem_readb(nes, addr & 0xFF);
}

// writes a byte to CPU address space
static inline void nes_mem_writeb(nes_t *nes, uint16_t addr, uint8_t val) {
  nes->cart.mapper.funcs.write(nes, addr, val);
}

// writes a word to CPU address space
static inline void nes_mem_writew(nes_t *nes, uint16_t addr, uint16_t val) {
  nes_mem_writeb(nes, addr, val & 0xFF);
  nes_mem_writeb(nes, addr + 1, (val >> 8) & 0xFF);
}

// pushes a byte to stack
static inline void nes_pushb(nes_t *nes, uint8_t val) {
  nes_mem_writeb(nes, 0x0100 | nes->cpu.s, val);
  nes->cpu.s -= 1;
}

// pushes a word to stack
static inline void nes_pushw(nes_t *nes, uint16_t val) {
  nes_pushb(nes, (val >> 8) & 0xFF);
  nes_pushb(nes, val & 0xFF);
}

// pops a byte from stack and returns it
static inline uint8_t nes_popb(nes_t *nes) {
  nes->cpu.s += 1;
  uint8_t res = nes_mem_readb(nes, 0x100 | (uint8_t)nes->cpu.s);

  return res;
}

// pops a word from stack and returns it
static inline uint16_t nes_popw(nes_t *nes) {
  uint16_t res = nes_popb(nes);
  res |= (uint16_t)nes_popb(nes) << 8;

  return res;
}

// reads byte at PC and increments PC
static inline uint8_t nes_mem_read_nextb(nes_t *nes) {
  uint8_t res = nes_mem_readb(nes, nes->cpu.pc);
  nes->cpu.pc += 1;

  return res;
}

// reads word at PC and increments PC
static inline uint16_t nes_mem_read_nextw(nes_t *nes) {
  uint16_t res = nes_mem_readw(nes, nes->cpu.pc);
  nes->cpu.pc += 2;

  return res;
}

// calculates JMI target address
static inline uint16_t nes_jmi_addr(nes_t *nes, uint16_t addr) {
  uint8_t lo = nes_mem_readb(nes, addr);
  uint8_t hi = nes_mem_readb(nes, (addr & 0xFF00) | ((addr + 1) & 0x00FF));

  return ((uint16_t)hi << 8) | lo;
}
