#pragma once

#include <string.h>

#include "nes_structs.h"
#include "nes_mem.h"
#include "nes_ppu.h"
#include "bitops.h"
#include "error.h"
#include "errcodes.h"

#define NES_VEC_NMI 0xFFFA // NMI vector address
#define NES_VEC_RESET 0xFFFC // RESET vector address
#define NES_VEC_IRQ 0xFFFE // IRQ vector address

// masks for P register flags
#define FLAG_MASK_C 0x01
#define FLAG_MASK_Z 0x02
#define FLAG_MASK_I 0x04
#define FLAG_MASK_D 0x08
#define FLAG_MASK_B 0x10
#define FLAG_MASK_UNUSED 0x20
#define FLAG_MASK_V 0x40
#define FLAG_MASK_N 0x80

// P register flags
// note: these are bit numbers, not masks
enum nes_cpu_flag {
  NES_CPU_FLAG_C, // C (Carry)
  NES_CPU_FLAG_Z, // Z (Zero)
  NES_CPU_FLAG_I, // I (Interrupt)
  NES_CPU_FLAG_D, // D (Decimal)
  NES_CPU_FLAG_B, // B (Break)
  NES_CPU_FLAG_UNUSED,
  NES_CPU_FLAG_V, // V (oVerflow)
  NES_CPU_FLAG_N, // N (Negative)
};

#if defined(DEBUG) && !defined(DEBUG_SDL)
#include <stdio.h>
#include "nes_cpu_debug.h"
#endif

static inline void nes_cpu_init(nes_cpu_t *cpu) {
  cpu->pc = 0xC000;
  cpu->s = 0xFD;

  cpu->a = 0x00;
  cpu->x = 0x00;
  cpu->y = 0x00;

  cpu->p = 0x24;
}

// calls NMI vector, consumes 7 cycles
static inline void nes_cpu_nmi(nes_t *nes) {
  nes_pushw(nes, nes->cpu.pc);
  nes_pushb(nes, nes->cpu.p);
  nes->cpu.p = BITMSET(nes->cpu.p, FLAG_MASK_I);
  nes->cpu.pc = nes_mem_readw(nes, NES_VEC_NMI);
  nes->cpu.cycle += 7;
}

// calls IRQ vector if allowed, consumes 7 cycles
static inline void nes_cpu_irq(nes_t *nes) {
  if (BITMGET(nes->cpu.p, FLAG_MASK_I)) return;
  nes_pushw(nes, nes->cpu.pc);
  nes_pushb(nes, nes->cpu.p);
  nes->cpu.p = BITMSET(nes->cpu.p, FLAG_MASK_I);
  nes->cpu.pc = nes_mem_readw(nes, NES_VEC_IRQ);
  nes->cpu.cycle += 7;
}

// checks if there's a page boundary between addresses a and b
// if so, sets the page cross flag
static inline void nes_cpu_pagecross(nes_t *nes, uint16_t a, uint16_t b) {
  if ((a & 0xFF00) != (b & 0xFF00))
    nes->cpu.pages_crossed = 1;
}

// consumes additional cycles for branching instructions and crossing pages
static inline void nes_cpu_add_branch_cycles(nes_t *nes, uint16_t pc_old) {
  // take one more cycle if crossed pages
  if ((pc_old & 0xFF00) != (nes->cpu.pc & 0xFF00))
    nes->cpu.cycle++;
  // take one more cycle for branching
  nes->cpu.cycle++;
}

// address translation functions for different addressing modes
// these read and return the address

// absolute
static inline uint16_t nes_a_abs(nes_t *nes) {
  return nes_mem_read_nextw(nes);
}

// absolute indexed by X
static inline uint16_t nes_a_abx(nes_t *nes) {
  uint16_t a = nes_a_abs(nes);
  uint16_t b = a + nes->cpu.x;
  nes_cpu_pagecross(nes, a, b);

  return b;
}

// absolute indexed by Y
static inline uint16_t nes_a_aby(nes_t *nes) {
  uint16_t a = nes_a_abs(nes);
  uint16_t b = a + nes->cpu.y;
  nes_cpu_pagecross(nes, a, b);

  return b;
}

// indexed indirect
static inline uint16_t nes_a_ndx(nes_t *nes) {
  return nes_mem_readw_zp(nes, nes_mem_read_nextb(nes) + nes->cpu.x);
}

// indirect indexed
static inline uint16_t nes_a_ndy(nes_t *nes) {
  uint16_t a = nes_mem_readw_zp(nes, nes_mem_read_nextb(nes));
  uint16_t b = a + nes->cpu.y;
  nes_cpu_pagecross(nes, a, b);

  return b;
}

// zero page
static inline uint16_t nes_a_zpg(nes_t *nes) {
  return nes_mem_read_nextb(nes);
}

// zero page indexed by X
static inline uint16_t nes_a_zpx(nes_t *nes) {
  return (nes_a_zpg(nes) + nes->cpu.x) & 0x00FF;
}

// zero page indexed by Y
static inline uint16_t nes_a_zpy(nes_t *nes) {
  return (nes_a_zpg(nes) + nes->cpu.y) & 0x00FF;
}

// memory reading functions using different addressing modes
// these calculate the address using previous set of functions and then
// read and return the value at that address

// absolute
static inline uint16_t nes_v_abs(nes_t *nes) {
  return nes_mem_readb(nes, nes_a_abs(nes));
}

// absolute indexed by X
static inline uint16_t nes_v_abx(nes_t *nes) {
  return nes_mem_readb(nes, nes_a_abx(nes));
}

// absolute indexed by Y
static inline uint16_t nes_v_aby(nes_t *nes) {
  return nes_mem_readb(nes, nes_a_aby(nes));
}

// accumulator (just returns a)
static inline uint16_t nes_v_acc(nes_t *nes) {
  return nes->cpu.a;
}

// absolute
static inline uint16_t nes_v_imm(nes_t *nes) {
  return nes_mem_read_nextb(nes);
}

// indexed indirect
static inline uint16_t nes_v_ndx(nes_t *nes) {
  return nes_mem_readb(nes, nes_a_ndx(nes));
}

// indirect indexed
static inline uint16_t nes_v_ndy(nes_t *nes) {
  return nes_mem_readb(nes, nes_a_ndy(nes));
}

// zero page
static inline uint16_t nes_v_zpg(nes_t *nes) {
  return nes_mem_readb_zp(nes, nes_a_zpg(nes));
}

// zero page indexed by X
static inline uint16_t nes_v_zpx(nes_t *nes) {
  return nes_mem_readb_zp(nes, nes_a_zpx(nes));
}

// zero page indexed by Y
static inline uint16_t nes_v_zpy(nes_t *nes) {
  return nes_mem_readb_zp(nes, nes_a_zpy(nes));
}

// sets Z and N flags based on given value
static inline void nes_cpu_set_zn(nes_t *nes, uint8_t val) {
  nes->cpu.p = BITMCHG(nes->cpu.p, FLAG_MASK_N, (val & 0x80));
  nes->cpu.p = BITMCHG(nes->cpu.p, FLAG_MASK_Z, !val);
}

// documented CPU instructions

// LDA: load operand into A
static inline void nes_op_lda(nes_t *nes, uint16_t val) {
  nes->cpu.a = val;
  nes_cpu_set_zn(nes, val);
}

// LDX: load operand into X
static inline void nes_op_ldx(nes_t *nes, uint16_t val) {
  nes->cpu.x = val;
  nes_cpu_set_zn(nes, val);
}

// LDY: load operand into Y
static inline void nes_op_ldy(nes_t *nes, uint16_t val) {
  nes->cpu.y = val;
  nes_cpu_set_zn(nes, val);
}

// STA: write A to memory
static inline void nes_op_sta(nes_t *nes, uint16_t addr) {
  nes_mem_writeb(nes, addr, nes->cpu.a);
}

// STX: write X to memory
static inline void nes_op_stx(nes_t *nes, uint16_t addr) {
  nes_mem_writeb(nes, addr, nes->cpu.x);
}

// STY: write Y to memory
static inline void nes_op_sty(nes_t *nes, uint16_t addr) {
  nes_mem_writeb(nes, addr, nes->cpu.y);
}

// ADC: add operand to A with carry (C flag)
static inline void nes_op_adc(nes_t *nes, uint16_t val) {
  uint16_t res = nes->cpu.a + val + ((nes->cpu.p & FLAG_MASK_C) ? 1 : 0);

  nes->cpu.p = BITMCHG(nes->cpu.p, FLAG_MASK_C, (res & 0x100));
  nes->cpu.p =
    BITMCHG(nes->cpu.p, FLAG_MASK_V,
                 !((nes->cpu.a ^ val) & 0x80) && ((nes->cpu.a ^ res) & 0x80));
  nes_cpu_set_zn(nes, res & 0xFF);

  nes->cpu.a = res;
}

// SBC: sub operand from A with carry
static inline void nes_op_sbc(nes_t *nes, uint16_t val) {
  uint16_t res = nes->cpu.a - val - ((nes->cpu.p & FLAG_MASK_C) ? 0 : 1);

  nes->cpu.p = BITMCHG(nes->cpu.p, FLAG_MASK_C, !(res & 0x100));
  nes->cpu.p =
    BITMCHG(nes->cpu.p, FLAG_MASK_V,
                 ((nes->cpu.a ^ val) & 0x80) && ((nes->cpu.a ^ res) & 0x80));
  nes_cpu_set_zn(nes, res & 0xFF);
  nes->cpu.a = res;
}

// compares two operands and sets C, Z, N flags based on result
static inline void nes_compare(nes_t *nes, uint8_t a, uint8_t b) {
  uint16_t res = a - b;

  nes->cpu.p = BITMCHG(nes->cpu.p, FLAG_MASK_C, !(res & 0x100));
  nes_cpu_set_zn(nes, res & 0xFF);
}

// CMP: compare A with operand
static inline void nes_op_cmp(nes_t *nes, uint16_t val) {
  nes_compare(nes, nes->cpu.a, val);
}

// CPX: compare X with operand
static inline void nes_op_cpx(nes_t *nes, uint16_t val) {
  nes_compare(nes, nes->cpu.x, val);
}

// CPY: compare Y with operand
static inline void nes_op_cpy(nes_t *nes, uint16_t val) {
  nes_compare(nes, nes->cpu.y, val);
}

// AND: bitwise AND A with operand
static inline void nes_op_and(nes_t *nes, uint16_t val) {
  uint8_t res = nes->cpu.a & val;

  nes_cpu_set_zn(nes, res);
  nes->cpu.a = res;
}

// ORA: bitwise OR A with operand
static inline void nes_op_ora(nes_t *nes, uint16_t val) {
  uint8_t res = nes->cpu.a | val;

  nes_cpu_set_zn(nes, res);
  nes->cpu.a = res;
}

// EOR: bitwise XOR A with operand
static inline void nes_op_eor(nes_t *nes, uint16_t val) {
  uint8_t res = nes->cpu.a ^ val;

  nes_cpu_set_zn(nes, res);
  nes->cpu.a = res;
}

// BIT: AND A with operand, set V, N, Z flags based on result
static inline void nes_op_bit(nes_t *nes, uint16_t val) {
  nes->cpu.p = BITMCHG(nes->cpu.p, FLAG_MASK_V, (val & 0x40));
  nes->cpu.p = BITMCHG(nes->cpu.p, FLAG_MASK_N, (val & 0x80));
  nes->cpu.p = BITMCHG(nes->cpu.p, FLAG_MASK_Z, !(val & nes->cpu.a));
}

// ROL: bitwise rotate operand left by 1
static inline void nes_op_rol(nes_t *nes, uint16_t addr) {
  uint8_t val = nes_mem_readb(nes, addr);
  uint8_t res = val << 1;

  if (BITMGET(nes->cpu.p, FLAG_MASK_C)) res |= 0x01;

  nes->cpu.p = BITMCHG(nes->cpu.p, FLAG_MASK_C, (val & 0x80));
  nes_cpu_set_zn(nes, res);
  nes_mem_writeb(nes, addr, res);
}

// ROR: bitwise rotate operand right by 1
static inline void nes_op_ror(nes_t *nes, uint16_t addr) {
  uint8_t val = nes_mem_readb(nes, addr);
  uint8_t res = val >> 1;

  if (BITMGET(nes->cpu.p, FLAG_MASK_C)) res |= 0x80;

  nes->cpu.p = BITMCHG(nes->cpu.p, FLAG_MASK_C, (val & 0x01));
  nes_cpu_set_zn(nes, res);
  nes_mem_writeb(nes, addr, res);
}

// ROLA: bitwise rotate A left
static inline void nes_op_rola(nes_t *nes, uint16_t val) {
  uint8_t res = val << 1;

  if (BITMGET(nes->cpu.p, FLAG_MASK_C)) res |= 0x01;

  nes->cpu.p = BITMCHG(nes->cpu.p, FLAG_MASK_C, (val & 0x80));
  nes_cpu_set_zn(nes, res);
  nes->cpu.a = res;
}

// RORA: bitwise rotate A right
static inline void nes_op_rora(nes_t *nes, uint16_t val) {
  uint8_t res = val >> 1;

  if (BITMGET(nes->cpu.p, FLAG_MASK_C)) res |= 0x80;

  nes->cpu.p = BITMCHG(nes->cpu.p, FLAG_MASK_C, (val & 0x01));
  nes_cpu_set_zn(nes, res);
  nes->cpu.a = res;
}

// ASL: arithmetic shift operand left
static inline void nes_op_asl(nes_t *nes, uint16_t addr) {
  uint8_t val = nes_mem_readb(nes, addr);
  uint8_t res = val << 1;

  nes->cpu.p = BITMCHG(nes->cpu.p, FLAG_MASK_C, (val & 0x80));
  nes_cpu_set_zn(nes, res);
  nes_mem_writeb(nes, addr, res);
}

// LSR: arithmetic shift operand right
static inline void nes_op_lsr(nes_t *nes, uint16_t addr) {
  uint8_t val = nes_mem_readb(nes, addr);
  uint8_t res = val >> 1;

  nes->cpu.p = BITMCHG(nes->cpu.p, FLAG_MASK_C, (val & 0x01));
  nes_cpu_set_zn(nes, res);
  nes_mem_writeb(nes, addr, res);
}

// ASLA: arithmetic shift A left
static inline void nes_op_asla(nes_t *nes, uint16_t val) {
  uint8_t res = val << 1;

  nes->cpu.p = BITMCHG(nes->cpu.p, FLAG_MASK_C, (val & 0x80));
  nes_cpu_set_zn(nes, res);
  nes->cpu.a = res;
}

// LSRA: arithmetic shift A right
static inline void nes_op_lsra(nes_t *nes, uint16_t val) {
  uint8_t res = val >> 1;

  nes->cpu.p = BITMCHG(nes->cpu.p, FLAG_MASK_C, (val & 0x01));
  nes_cpu_set_zn(nes, res);
  nes->cpu.a = res;
}

// INC: increment operand
static inline void nes_op_inc(nes_t *nes, uint16_t addr) {
  uint8_t res = nes_mem_readb(nes, addr) + 1;

  nes_cpu_set_zn(nes, res);

  nes_mem_writeb(nes, addr, res);
}

// DEC: decrement operand
static inline void nes_op_dec(nes_t *nes, uint16_t addr) {
  uint8_t res = nes_mem_readb(nes, addr) - 1;

  nes_cpu_set_zn(nes, res);

  nes_mem_writeb(nes, addr, res);
}

// INX: increment X
static inline void nes_op_inx(nes_t *nes) {
  uint8_t res = nes->cpu.x + 1;

  nes_cpu_set_zn(nes, res);

  nes->cpu.x = res;
}

// DEX: decrement X
static inline void nes_op_dex(nes_t *nes) {
  uint8_t res = nes->cpu.x - 1;

  nes_cpu_set_zn(nes, res);

  nes->cpu.x = res;
}

// INY: increment Y
static inline void nes_op_iny(nes_t *nes) {
  uint8_t res = nes->cpu.y + 1;

  nes_cpu_set_zn(nes, res);

  nes->cpu.y = res;
}

// DEY: decrement Y
static inline void nes_op_dey(nes_t *nes) {
  uint8_t res = nes->cpu.y - 1;

  nes_cpu_set_zn(nes, res);

  nes->cpu.y = res;
}

// helper macro for register-to-register transfer operations
// copies from r1 to r2 and sets Z and N flags based on r1
#define NES_TRANSFER(nes, r1, r2)   \
  nes_cpu_set_zn(nes, nes->cpu.r1); \
  nes->cpu.r2 = nes->cpu.r1;

// TAX: copy A to X
static inline void nes_op_tax(nes_t *nes) {
  NES_TRANSFER(nes, a, x);
}

// TAY: copy A to Y
static inline void nes_op_tay(nes_t *nes) {
  NES_TRANSFER(nes, a, y);
}

// TXA: copy X to A
static inline void nes_op_txa(nes_t *nes) {
  NES_TRANSFER(nes, x, a);
}

// TYA: copy Y to A
static inline void nes_op_tya(nes_t *nes) {
  NES_TRANSFER(nes, y, a);
}

// TXS: copy X to S
static inline void nes_op_txs(nes_t *nes) {
  nes->cpu.s = nes->cpu.x;
}

// TSX: copy S to X
static inline void nes_op_tsx(nes_t *nes) {
  NES_TRANSFER(nes, s, x);
}

// CLC: clear C flag
static inline void nes_op_clc(nes_t *nes) {
  nes->cpu.p = BITMCLR(nes->cpu.p, FLAG_MASK_C);
}

// SEC: set C flag
static inline void nes_op_sec(nes_t *nes) {
  nes->cpu.p = BITMSET(nes->cpu.p, FLAG_MASK_C);
}

// CLI: clear I flag
static inline void nes_op_cli(nes_t *nes) {
  nes->cpu.p = BITMCLR(nes->cpu.p, FLAG_MASK_I);
}

// SEI: set I flag
static inline void nes_op_sei(nes_t *nes) {
  nes->cpu.p = BITMSET(nes->cpu.p, FLAG_MASK_I);
}

// CLV: clear V flag
static inline void nes_op_clv(nes_t *nes) {
  nes->cpu.p = BITMCLR(nes->cpu.p, FLAG_MASK_V);
}

// CLD: clear D flag
static inline void nes_op_cld(nes_t *nes) {
  nes->cpu.p = BITMCLR(nes->cpu.p, FLAG_MASK_D);
}

// SED: set D flag
static inline void nes_op_sed(nes_t *nes) {
  nes->cpu.p = BITMSET(nes->cpu.p, FLAG_MASK_D);
}

// base function for relative branching instructions
// adds additional branch cycles where needed
static inline void nes_branch_jmp(nes_t *nes, uint8_t flag) {
  uint8_t offset = nes_mem_read_nextb(nes);
  uint16_t pc_old = nes->cpu.pc;

  if (flag) {
    nes->cpu.pc += (int8_t)offset;
    nes_cpu_add_branch_cycles(nes, pc_old);
  }
}

// BPL: branch if N is 0
static inline void nes_op_bpl(nes_t *nes) {
  nes_branch_jmp(nes, !BITMGET(nes->cpu.p, FLAG_MASK_N));
}

// BML: branch if N is 1
static inline void nes_op_bmi(nes_t *nes) {
  nes_branch_jmp(nes, BITMGET(nes->cpu.p, FLAG_MASK_N));
}

// BVC: branch if V is 0
static inline void nes_op_bvc(nes_t *nes) {
  nes_branch_jmp(nes, !BITMGET(nes->cpu.p, FLAG_MASK_V));
}

// BVS: branch if V is 1
static inline void nes_op_bvs(nes_t *nes) {
  nes_branch_jmp(nes, BITMGET(nes->cpu.p, FLAG_MASK_V));
}

// BCC: branch if C is 0
static inline void nes_op_bcc(nes_t *nes) {
  nes_branch_jmp(nes, !BITMGET(nes->cpu.p, FLAG_MASK_C));
}

// BCS: branch if C is 1
static inline void nes_op_bcs(nes_t *nes) {
  nes_branch_jmp(nes, BITMGET(nes->cpu.p, FLAG_MASK_C));
}

// BNE: branch if Z is 0
static inline void nes_op_bne(nes_t *nes) {
  nes_branch_jmp(nes, !BITMGET(nes->cpu.p, FLAG_MASK_Z));
}

// BEQ: branch if Z is 1
static inline void nes_op_beq(nes_t *nes) {
  nes_branch_jmp(nes, BITMGET(nes->cpu.p, FLAG_MASK_Z));
}

// JMP: absolute jump
static inline void nes_op_jmp(nes_t *nes) {
  nes->cpu.pc = nes_mem_read_nextw(nes);
}

// JMI: relative jump
static inline void nes_op_jmi(nes_t *nes) {
  nes->cpu.pc = nes_jmi_addr(nes, nes_mem_read_nextw(nes));
}

// JSR: subroutine call
static inline void nes_op_jsr(nes_t *nes) {
  uint16_t addr = nes_mem_read_nextw(nes);

  nes_pushw(nes, nes->cpu.pc - 1);
  nes->cpu.pc = addr;
}

// RTS: return from subroutine
static inline void nes_op_rts(nes_t *nes) {
  nes->cpu.pc = nes_popw(nes) + 1;
}

// BRK: software interrupt
static inline void nes_op_brk(nes_t *nes) {
  if (!BITMGET(nes->cpu.p, FLAG_MASK_I)) {
    nes_pushw(nes, nes->cpu.pc - 1);
    nes_pushb(nes, nes->cpu.p);

    nes->cpu.p = BITMSET(nes->cpu.p, FLAG_MASK_I);
    nes->cpu.pc = nes_mem_readw(nes, NES_VEC_IRQ);
  }
}

// RTI: return from interrupt
static inline void nes_op_rti(nes_t *nes) {
  nes->cpu.p = (nes_popb(nes) | 0x30) - 0x10;
  nes->cpu.pc = nes_popw(nes);
}

// PHA: push A to stack
static inline void nes_op_pha(nes_t *nes) {
  nes_pushb(nes, nes->cpu.a);
}

// PLA: pop A from stack
static inline void nes_op_pla(nes_t *nes) {
  nes->cpu.a = nes_popb(nes);

  nes_cpu_set_zn(nes, nes->cpu.a);
}

// PHP: push P to stack
static inline void nes_op_php(nes_t *nes) {
  nes_pushb(nes, nes->cpu.p | FLAG_MASK_B);
}

// PLP: pop P from stack
static inline void nes_op_plp(nes_t *nes) {
  nes->cpu.p = (nes_popb(nes) | 0x30) - 0x10;
}

// NOP: no-op
static inline void nes_op_nop(nes_t *nes) {
  return;
}

// undocumented (illegal) instructions

// ANC: AND imm, sets N and Z flags based on result
static inline void nes_op_anc(nes_t *nes, uint16_t val) {
  return;
}

// ALR: AND imm + LSR A
static inline void nes_op_alr(nes_t *nes, uint16_t val) {
  uint8_t res = (nes->cpu.a & val) >> 1;

  nes->cpu.p = BITMCHG(nes->cpu.p, FLAG_MASK_C, (val & 0x01));
  nes_cpu_set_zn(nes, res);
  nes->cpu.a = res;
}

// ARR: AND imm + ROR A, sets C from bit 6 and V from bit 6 xor bit 5
static inline void nes_op_arr(nes_t *nes, uint16_t val) {
  uint8_t res = (nes->cpu.a & val) >> 1;

  if (BITMGET(nes->cpu.p, FLAG_MASK_C)) res |= 0x80;

  nes->cpu.p = BITMCHG(nes->cpu.p, FLAG_MASK_C, (res & 0x40));
  nes->cpu.p = BITMCHG(nes->cpu.p, FLAG_MASK_C, (res & 0x40) ^ (res & 0x20));
  nes_cpu_set_zn(nes, res);
  nes->cpu.a = res;
}

// AXS: X = (A & X) - operand, updates N, Z, C
static inline void nes_op_axs(nes_t *nes, uint16_t val) {
  uint16_t res = (nes->cpu.a & nes->cpu.x) - val;

  nes->cpu.p = BITMCHG(nes->cpu.p, FLAG_MASK_C, !(res & 0x100));
  nes->cpu.p =
    BITMCHG(nes->cpu.p, FLAG_MASK_V,
                 ((nes->cpu.a ^ val) & 0x80) && ((nes->cpu.a ^ res) & 0x80));
  nes_cpu_set_zn(nes, res & 0xFF);
  nes->cpu.x = res;
}

// LAX: LDA + TAX
static inline void nes_op_lax(nes_t *nes, uint16_t val) {
  nes->cpu.a = val;
  nes->cpu.x = val;
  nes_cpu_set_zn(nes, val);
}

// SAX: store (A & X), flags unaffected
static inline void nes_op_sax(nes_t *nes, uint16_t addr) {
  nes_mem_writeb(nes, addr, nes->cpu.a & nes->cpu.x);
}

// DCP: DEC + CMP
static inline void nes_op_dcp(nes_t *nes, uint16_t addr) {
  nes_op_dec(nes, addr);
  nes_op_cmp(nes, nes_mem_readb(nes, addr));
}

// ISC: INC + SBC
static inline void nes_op_isb(nes_t *nes, uint16_t addr) {
  nes_op_inc(nes, addr);
  nes_op_sbc(nes, nes_mem_readb(nes, addr));
}

// RLA: ROL + AND
static inline void nes_op_rla(nes_t *nes, uint16_t addr) {
  nes_op_rol(nes, addr);
  nes_op_and(nes, nes_mem_readb(nes, addr));
}

// RRA: ROR + ADC
static inline void nes_op_rra(nes_t *nes, uint16_t addr) {
  nes_op_ror(nes, addr);
  nes_op_adc(nes, nes_mem_readb(nes, addr));
}

// SLO: ASL + ORA
static inline void nes_op_slo(nes_t *nes, uint16_t addr) {
  nes_op_asl(nes, addr);
  nes_op_ora(nes, nes_mem_readb(nes, addr));
}

// SRE: LSR + EOR
static inline void nes_op_sre(nes_t *nes, uint16_t addr) {
  nes_op_lsr(nes, addr);
  nes_op_eor(nes, nes_mem_readb(nes, addr));
}

// SKB: read byte and ignore (2-cycle no-op)
static inline void nes_op_skb(nes_t *nes, uint16_t val) {
  return;
}

// IGN: read byte and ignore (3 to 5-cycle no-op)
static inline void nes_op_ign(nes_t *nes, uint16_t addr) {
  return;
}

// reads and executes next instruction
static inline uint32_t nes_cpu_op(nes_t *nes) {
  // cycle costs for each opcode
  static uint8_t op_cycles[256] = {
    7, 6, 2, 8, 3, 3, 5, 5, 3, 2, 2, 2, 4, 4, 6, 6, 2, 5, 2, 8, 4, 4, 6, 6,
    2, 4, 2, 7, 4, 4, 7, 7, 6, 6, 2, 8, 3, 3, 5, 5, 4, 2, 2, 2, 4, 4, 6, 6,
    6, 6, 2, 8, 3, 3, 5, 5, 3, 2, 2, 2, 3, 4, 6, 6, 2, 5, 2, 8, 4, 4, 6, 6,
    2, 4, 2, 7, 4, 4, 7, 7, 2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
    6, 6, 2, 8, 3, 3, 5, 5, 4, 2, 2, 2, 5, 4, 6, 6, 2, 5, 2, 8, 4, 4, 6, 6,
    2, 4, 2, 7, 4, 4, 7, 7, 2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 2, 4, 4, 4, 4,
    2, 6, 2, 6, 4, 4, 4, 4, 2, 5, 2, 5, 5, 5, 5, 5, 2, 6, 2, 6, 3, 3, 3, 3,
    2, 2, 2, 2, 4, 4, 4, 4, 2, 5, 2, 5, 4, 4, 4, 4, 2, 4, 2, 4, 4, 4, 4, 4,
    2, 6, 2, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6, 2, 5, 2, 8, 4, 4, 6, 6,
    2, 4, 2, 7, 4, 4, 7, 7, 2, 6, 3, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6,
    2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
  };

  // page cross costs for each opcode
  static uint8_t op_page_cycles[256] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0,
    0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0,
    0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0,
    0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0,
  };

  nes->cpu.pages_crossed = 0;

  if (nes->cpu.stall) {
    nes->cpu.stall--;
    return 1;
  }

#if defined(DEBUG) && !defined(DEBUG_SDL)
  nes_cpu_debug_print_nesulator(nes, stdout);
#endif

  uint64_t cycle_old = nes->cpu.cycle;

  uint8_t opcode = nes_mem_read_nextb(nes);
  switch (opcode) {
    case 0xA1: nes_op_lda(nes, nes_v_ndx(nes)); break;
    case 0xA5: nes_op_lda(nes, nes_v_zpg(nes)); break;
    case 0xA9: nes_op_lda(nes, nes_v_imm(nes)); break;
    case 0xAD: nes_op_lda(nes, nes_v_abs(nes)); break;
    case 0xB1: nes_op_lda(nes, nes_v_ndy(nes)); break;
    case 0xB5: nes_op_lda(nes, nes_v_zpx(nes)); break;
    case 0xB9: nes_op_lda(nes, nes_v_aby(nes)); break;
    case 0xBD: nes_op_lda(nes, nes_v_abx(nes)); break;

    case 0xA2: nes_op_ldx(nes, nes_v_imm(nes)); break;
    case 0xA6: nes_op_ldx(nes, nes_v_zpg(nes)); break;
    case 0xB6: nes_op_ldx(nes, nes_v_zpy(nes)); break;
    case 0xAE: nes_op_ldx(nes, nes_v_abs(nes)); break;
    case 0xBE: nes_op_ldx(nes, nes_v_aby(nes)); break;

    case 0xA0: nes_op_ldy(nes, nes_v_imm(nes)); break;
    case 0xA4: nes_op_ldy(nes, nes_v_zpg(nes)); break;
    case 0xB4: nes_op_ldy(nes, nes_v_zpx(nes)); break;
    case 0xAC: nes_op_ldy(nes, nes_v_abs(nes)); break;
    case 0xBC: nes_op_ldy(nes, nes_v_abx(nes)); break;

    case 0x81: nes_op_sta(nes, nes_a_ndx(nes)); break;
    case 0x85: nes_op_sta(nes, nes_a_zpg(nes)); break;
    case 0x8D: nes_op_sta(nes, nes_a_abs(nes)); break;
    case 0x91: nes_op_sta(nes, nes_a_ndy(nes)); break;
    case 0x95: nes_op_sta(nes, nes_a_zpx(nes)); break;
    case 0x99: nes_op_sta(nes, nes_a_aby(nes)); break;
    case 0x9D: nes_op_sta(nes, nes_a_abx(nes)); break;

    case 0x86: nes_op_stx(nes, nes_a_zpg(nes)); break;
    case 0x8E: nes_op_stx(nes, nes_a_abs(nes)); break;
    case 0x96: nes_op_stx(nes, nes_a_zpy(nes)); break;

    case 0x84: nes_op_sty(nes, nes_a_zpg(nes)); break;
    case 0x8C: nes_op_sty(nes, nes_a_abs(nes)); break;
    case 0x94: nes_op_sty(nes, nes_a_zpx(nes)); break;

    case 0x69: nes_op_adc(nes, nes_v_imm(nes)); break;
    case 0x65: nes_op_adc(nes, nes_v_zpg(nes)); break;
    case 0x75: nes_op_adc(nes, nes_v_zpx(nes)); break;
    case 0x6D: nes_op_adc(nes, nes_v_abs(nes)); break;
    case 0x7D: nes_op_adc(nes, nes_v_abx(nes)); break;
    case 0x79: nes_op_adc(nes, nes_v_aby(nes)); break;
    case 0x61: nes_op_adc(nes, nes_v_ndx(nes)); break;
    case 0x71: nes_op_adc(nes, nes_v_ndy(nes)); break;

    case 0xE9: nes_op_sbc(nes, nes_v_imm(nes)); break;
    case 0xE5: nes_op_sbc(nes, nes_v_zpg(nes)); break;
    case 0xF5: nes_op_sbc(nes, nes_v_zpx(nes)); break;
    case 0xED: nes_op_sbc(nes, nes_v_abs(nes)); break;
    case 0xFD: nes_op_sbc(nes, nes_v_abx(nes)); break;
    case 0xF9: nes_op_sbc(nes, nes_v_aby(nes)); break;
    case 0xE1: nes_op_sbc(nes, nes_v_ndx(nes)); break;
    case 0xF1: nes_op_sbc(nes, nes_v_ndy(nes)); break;

    case 0xC9: nes_op_cmp(nes, nes_v_imm(nes)); break;
    case 0xC5: nes_op_cmp(nes, nes_v_zpg(nes)); break;
    case 0xD5: nes_op_cmp(nes, nes_v_zpx(nes)); break;
    case 0xCD: nes_op_cmp(nes, nes_v_abs(nes)); break;
    case 0xDD: nes_op_cmp(nes, nes_v_abx(nes)); break;
    case 0xD9: nes_op_cmp(nes, nes_v_aby(nes)); break;
    case 0xC1: nes_op_cmp(nes, nes_v_ndx(nes)); break;
    case 0xD1: nes_op_cmp(nes, nes_v_ndy(nes)); break;

    case 0xE0: nes_op_cpx(nes, nes_v_imm(nes)); break;
    case 0xE4: nes_op_cpx(nes, nes_v_zpg(nes)); break;
    case 0xEC: nes_op_cpx(nes, nes_v_abs(nes)); break;

    case 0xC0: nes_op_cpy(nes, nes_v_imm(nes)); break;
    case 0xC4: nes_op_cpy(nes, nes_v_zpg(nes)); break;
    case 0xCC: nes_op_cpy(nes, nes_v_abs(nes)); break;

    case 0x29: nes_op_and(nes, nes_v_imm(nes)); break;
    case 0x25: nes_op_and(nes, nes_v_zpg(nes)); break;
    case 0x35: nes_op_and(nes, nes_v_zpx(nes)); break;
    case 0x2D: nes_op_and(nes, nes_v_abs(nes)); break;
    case 0x3D: nes_op_and(nes, nes_v_abx(nes)); break;
    case 0x39: nes_op_and(nes, nes_v_aby(nes)); break;
    case 0x21: nes_op_and(nes, nes_v_ndx(nes)); break;
    case 0x31: nes_op_and(nes, nes_v_ndy(nes)); break;

    case 0x09: nes_op_ora(nes, nes_v_imm(nes)); break;
    case 0x05: nes_op_ora(nes, nes_v_zpg(nes)); break;
    case 0x15: nes_op_ora(nes, nes_v_zpx(nes)); break;
    case 0x0D: nes_op_ora(nes, nes_v_abs(nes)); break;
    case 0x1D: nes_op_ora(nes, nes_v_abx(nes)); break;
    case 0x19: nes_op_ora(nes, nes_v_aby(nes)); break;
    case 0x01: nes_op_ora(nes, nes_v_ndx(nes)); break;
    case 0x11: nes_op_ora(nes, nes_v_ndy(nes)); break;

    case 0x49: nes_op_eor(nes, nes_v_imm(nes)); break;
    case 0x45: nes_op_eor(nes, nes_v_zpg(nes)); break;
    case 0x55: nes_op_eor(nes, nes_v_zpx(nes)); break;
    case 0x4D: nes_op_eor(nes, nes_v_abs(nes)); break;
    case 0x5D: nes_op_eor(nes, nes_v_abx(nes)); break;
    case 0x59: nes_op_eor(nes, nes_v_aby(nes)); break;
    case 0x41: nes_op_eor(nes, nes_v_ndx(nes)); break;
    case 0x51: nes_op_eor(nes, nes_v_ndy(nes)); break;

    case 0x24: nes_op_bit(nes, nes_v_zpg(nes)); break;
    case 0x2C: nes_op_bit(nes, nes_v_abs(nes)); break;

    case 0x2A: nes_op_rola(nes, nes_v_acc(nes)); break;
    case 0x26: nes_op_rol(nes, nes_a_zpg(nes)); break;
    case 0x36: nes_op_rol(nes, nes_a_zpx(nes)); break;
    case 0x2E: nes_op_rol(nes, nes_a_abs(nes)); break;
    case 0x3E: nes_op_rol(nes, nes_a_abx(nes)); break;

    case 0x6A: nes_op_rora(nes, nes_v_acc(nes)); break;
    case 0x66: nes_op_ror(nes, nes_a_zpg(nes)); break;
    case 0x76: nes_op_ror(nes, nes_a_zpx(nes)); break;
    case 0x6E: nes_op_ror(nes, nes_a_abs(nes)); break;
    case 0x7E: nes_op_ror(nes, nes_a_abx(nes)); break;

    case 0x0A: nes_op_asla(nes, nes_v_acc(nes)); break;
    case 0x06: nes_op_asl(nes, nes_a_zpg(nes)); break;
    case 0x16: nes_op_asl(nes, nes_a_zpx(nes)); break;
    case 0x0E: nes_op_asl(nes, nes_a_abs(nes)); break;
    case 0x1E: nes_op_asl(nes, nes_a_abx(nes)); break;

    case 0x4A: nes_op_lsra(nes, nes_v_acc(nes)); break;
    case 0x46: nes_op_lsr(nes, nes_a_zpg(nes)); break;
    case 0x56: nes_op_lsr(nes, nes_a_zpx(nes)); break;
    case 0x4E: nes_op_lsr(nes, nes_a_abs(nes)); break;
    case 0x5E: nes_op_lsr(nes, nes_a_abx(nes)); break;

    case 0xE6: nes_op_inc(nes, nes_a_zpg(nes)); break;
    case 0xF6: nes_op_inc(nes, nes_a_zpx(nes)); break;
    case 0xEE: nes_op_inc(nes, nes_a_abs(nes)); break;
    case 0xFE: nes_op_inc(nes, nes_a_abx(nes)); break;

    case 0xC6: nes_op_dec(nes, nes_a_zpg(nes)); break;
    case 0xD6: nes_op_dec(nes, nes_a_zpx(nes)); break;
    case 0xCE: nes_op_dec(nes, nes_a_abs(nes)); break;
    case 0xDE: nes_op_dec(nes, nes_a_abx(nes)); break;

    case 0xE8: nes_op_inx(nes); break;
    case 0xCA: nes_op_dex(nes); break;
    case 0xC8: nes_op_iny(nes); break;
    case 0x88: nes_op_dey(nes); break;

    case 0xAA: nes_op_tax(nes); break;
    case 0xA8: nes_op_tay(nes); break;
    case 0x8A: nes_op_txa(nes); break;
    case 0x98: nes_op_tya(nes); break;
    case 0x9A: nes_op_txs(nes); break;
    case 0xBA: nes_op_tsx(nes); break;

    case 0x18: nes_op_clc(nes); break;
    case 0x38: nes_op_sec(nes); break;
    case 0x58: nes_op_cli(nes); break;
    case 0x78: nes_op_sei(nes); break;
    case 0xB8: nes_op_clv(nes); break;
    case 0xD8: nes_op_cld(nes); break;
    case 0xF8: nes_op_sed(nes); break;

    case 0x10: nes_op_bpl(nes); break;
    case 0x30: nes_op_bmi(nes); break;
    case 0x50: nes_op_bvc(nes); break;
    case 0x70: nes_op_bvs(nes); break;
    case 0x90: nes_op_bcc(nes); break;
    case 0xB0: nes_op_bcs(nes); break;
    case 0xD0: nes_op_bne(nes); break;
    case 0xF0: nes_op_beq(nes); break;

    case 0x4C: nes_op_jmp(nes); break;
    case 0x6C: nes_op_jmi(nes); break;

    case 0x20: nes_op_jsr(nes); break;
    case 0x60: nes_op_rts(nes); break;
    case 0x00: nes_op_brk(nes); break;
    case 0x40: nes_op_rti(nes); break;

    case 0x48: nes_op_pha(nes); break;
    case 0x68: nes_op_pla(nes); break;
    case 0x08: nes_op_php(nes); break;
    case 0x28: nes_op_plp(nes); break;

    case 0x1A: nes_op_nop(nes); break;
    case 0x3A: nes_op_nop(nes); break;
    case 0x5A: nes_op_nop(nes); break;
    case 0x7A: nes_op_nop(nes); break;
    case 0xDA: nes_op_nop(nes); break;
    case 0xEA: nes_op_nop(nes); break;
    case 0xFA: nes_op_nop(nes); break;

    case 0x0B: nes_op_anc(nes, nes_v_imm(nes)); break;
    case 0x4B: nes_op_alr(nes, nes_v_imm(nes)); break;
    case 0x6B: nes_op_arr(nes, nes_v_imm(nes)); break;
    case 0xCB: nes_op_axs(nes, nes_v_imm(nes)); break;

    case 0xA3: nes_op_lax(nes, nes_v_ndx(nes)); break;
    case 0xA7: nes_op_lax(nes, nes_v_zpg(nes)); break;
    case 0xAF: nes_op_lax(nes, nes_v_abs(nes)); break;
    case 0xB3: nes_op_lax(nes, nes_v_ndy(nes)); break;
    case 0xB7: nes_op_lax(nes, nes_v_zpy(nes)); break;
    case 0xBF: nes_op_lax(nes, nes_v_aby(nes)); break;

    case 0x83: nes_op_sax(nes, nes_a_ndx(nes)); break;
    case 0x87: nes_op_sax(nes, nes_a_zpg(nes)); break;
    case 0x8F: nes_op_sax(nes, nes_a_abs(nes)); break;
    case 0x97: nes_op_sax(nes, nes_a_zpy(nes)); break;

    case 0xC3: nes_op_dcp(nes, nes_a_ndx(nes)); break;
    case 0xC7: nes_op_dcp(nes, nes_a_zpg(nes)); break;
    case 0xCF: nes_op_dcp(nes, nes_a_abs(nes)); break;
    case 0xD3: nes_op_dcp(nes, nes_a_ndy(nes)); break;
    case 0xD7: nes_op_dcp(nes, nes_a_zpx(nes)); break;
    case 0xDB: nes_op_dcp(nes, nes_a_aby(nes)); break;
    case 0xDF: nes_op_dcp(nes, nes_a_abx(nes)); break;

    case 0xE3: nes_op_isb(nes, nes_a_ndx(nes)); break;
    case 0xE7: nes_op_isb(nes, nes_a_zpg(nes)); break;
    case 0xEF: nes_op_isb(nes, nes_a_abs(nes)); break;
    case 0xF3: nes_op_isb(nes, nes_a_ndy(nes)); break;
    case 0xF7: nes_op_isb(nes, nes_a_zpx(nes)); break;
    case 0xFB: nes_op_isb(nes, nes_a_aby(nes)); break;
    case 0xFF: nes_op_isb(nes, nes_a_abx(nes)); break;

    case 0x23: nes_op_rla(nes, nes_a_ndx(nes)); break;
    case 0x27: nes_op_rla(nes, nes_a_zpg(nes)); break;
    case 0x2F: nes_op_rla(nes, nes_a_abs(nes)); break;
    case 0x33: nes_op_rla(nes, nes_a_ndy(nes)); break;
    case 0x37: nes_op_rla(nes, nes_a_zpx(nes)); break;
    case 0x3B: nes_op_rla(nes, nes_a_aby(nes)); break;
    case 0x3F: nes_op_rla(nes, nes_a_abx(nes)); break;

    case 0x63: nes_op_rra(nes, nes_a_ndx(nes)); break;
    case 0x67: nes_op_rra(nes, nes_a_zpg(nes)); break;
    case 0x6F: nes_op_rra(nes, nes_a_abs(nes)); break;
    case 0x73: nes_op_rra(nes, nes_a_ndy(nes)); break;
    case 0x77: nes_op_rra(nes, nes_a_zpx(nes)); break;
    case 0x7B: nes_op_rra(nes, nes_a_aby(nes)); break;
    case 0x7F: nes_op_rra(nes, nes_a_abx(nes)); break;

    case 0x03: nes_op_slo(nes, nes_a_ndx(nes)); break;
    case 0x07: nes_op_slo(nes, nes_a_zpg(nes)); break;
    case 0x0F: nes_op_slo(nes, nes_a_abs(nes)); break;
    case 0x13: nes_op_slo(nes, nes_a_ndy(nes)); break;
    case 0x17: nes_op_slo(nes, nes_a_zpx(nes)); break;
    case 0x1B: nes_op_slo(nes, nes_a_aby(nes)); break;
    case 0x1F: nes_op_slo(nes, nes_a_abx(nes)); break;

    case 0x43: nes_op_sre(nes, nes_a_ndx(nes)); break;
    case 0x47: nes_op_sre(nes, nes_a_zpg(nes)); break;
    case 0x4F: nes_op_sre(nes, nes_a_abs(nes)); break;
    case 0x53: nes_op_sre(nes, nes_a_ndy(nes)); break;
    case 0x57: nes_op_sre(nes, nes_a_zpx(nes)); break;
    case 0x5B: nes_op_sre(nes, nes_a_aby(nes)); break;
    case 0x5F: nes_op_sre(nes, nes_a_abx(nes)); break;

    case 0xEB: nes_op_sbc(nes, nes_v_imm(nes)); break;

    case 0x80: nes_op_skb(nes, nes_v_imm(nes)); break;
    case 0x82: nes_op_skb(nes, nes_v_imm(nes)); break;
    case 0x89: nes_op_skb(nes, nes_v_imm(nes)); break;
    case 0xC2: nes_op_skb(nes, nes_v_imm(nes)); break;
    case 0xE2: nes_op_skb(nes, nes_v_imm(nes)); break;

    case 0x0C: nes_op_ign(nes, nes_a_abs(nes)); break;
    case 0x1C: nes_op_ign(nes, nes_a_abx(nes)); break;
    case 0x3C: nes_op_ign(nes, nes_a_abx(nes)); break;
    case 0x5C: nes_op_ign(nes, nes_a_abx(nes)); break;
    case 0x7C: nes_op_ign(nes, nes_a_abx(nes)); break;
    case 0xDC: nes_op_ign(nes, nes_a_abx(nes)); break;
    case 0xFC: nes_op_ign(nes, nes_a_abx(nes)); break;
    case 0x04: nes_op_ign(nes, nes_a_zpg(nes)); break;
    case 0x44: nes_op_ign(nes, nes_a_zpg(nes)); break;
    case 0x64: nes_op_ign(nes, nes_a_zpg(nes)); break;
    case 0x14: nes_op_ign(nes, nes_a_zpx(nes)); break;
    case 0x34: nes_op_ign(nes, nes_a_zpx(nes)); break;
    case 0x54: nes_op_ign(nes, nes_a_zpx(nes)); break;
    case 0x74: nes_op_ign(nes, nes_a_zpx(nes)); break;
    case 0xD4: nes_op_ign(nes, nes_a_zpx(nes)); break;
    case 0xF4: nes_op_ign(nes, nes_a_zpx(nes)); break;

    default:
      fprintf(stderr, "Unimplemented or illegal instruction 0x%02X at 0x%04X\n",
        opcode, nes->cpu.pc - 1);
  };

  nes->cpu.cycle += op_cycles[opcode];
  if (nes->cpu.pages_crossed)
    nes->cpu.cycle += op_page_cycles[opcode];
  return nes->cpu.cycle - cycle_old;
}
