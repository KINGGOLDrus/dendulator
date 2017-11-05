#pragma once

#include <stdio.h>

#include "nes_structs.h"
#include "nes_mem.h"
#include "bitops.h"

enum nes_cpu_debug_ins_addr_mode {
  NES_ADDR_MODE_ABS,  // a
  NES_ADDR_MODE_ABX,  // a,x
  NES_ADDR_MODE_ABY,  // a,y
  NES_ADDR_MODE_ACC,  //
  NES_ADDR_MODE_IMM,  //
  NES_ADDR_MODE_IND,  //
  NES_ADDR_MODE_NDX,  // (d,x)
  NES_ADDR_MODE_NDY,  // (d),y
  NES_ADDR_MODE_REL,  // *+d
  NES_ADDR_MODE_ZPG,  // d
  NES_ADDR_MODE_ZPX,  // d,x
  NES_ADDR_MODE_ZPY,  // d,y
};

typedef struct {
  char *txt;
  int mode;
} nes_cpu_debug_op_info_t;

static inline void nes_cpu_debug_print_op_full(nes_t *nes, FILE *stream) {
  static nes_cpu_debug_op_info_t op_info[] = {
    [0xA1] = (nes_cpu_debug_op_info_t){" LDA", NES_ADDR_MODE_NDX},
    [0xA5] = (nes_cpu_debug_op_info_t){" LDA", NES_ADDR_MODE_ZPG},
    [0xA9] = (nes_cpu_debug_op_info_t){" LDA", NES_ADDR_MODE_IMM},
    [0xAD] = (nes_cpu_debug_op_info_t){" LDA", NES_ADDR_MODE_ABS},
    [0xB1] = (nes_cpu_debug_op_info_t){" LDA", NES_ADDR_MODE_NDY},
    [0xB5] = (nes_cpu_debug_op_info_t){" LDA", NES_ADDR_MODE_ZPX},
    [0xB9] = (nes_cpu_debug_op_info_t){" LDA", NES_ADDR_MODE_ABY},
    [0xBD] = (nes_cpu_debug_op_info_t){" LDA", NES_ADDR_MODE_ABX},
    [0xA2] = (nes_cpu_debug_op_info_t){" LDX", NES_ADDR_MODE_IMM},
    [0xA6] = (nes_cpu_debug_op_info_t){" LDX", NES_ADDR_MODE_ZPG},
    [0xB6] = (nes_cpu_debug_op_info_t){" LDX", NES_ADDR_MODE_ZPY},
    [0xAE] = (nes_cpu_debug_op_info_t){" LDX", NES_ADDR_MODE_ABS},
    [0xBE] = (nes_cpu_debug_op_info_t){" LDX", NES_ADDR_MODE_ABY},
    [0xA0] = (nes_cpu_debug_op_info_t){" LDY", NES_ADDR_MODE_IMM},
    [0xA4] = (nes_cpu_debug_op_info_t){" LDY", NES_ADDR_MODE_ZPG},
    [0xB4] = (nes_cpu_debug_op_info_t){" LDY", NES_ADDR_MODE_ZPX},
    [0xAC] = (nes_cpu_debug_op_info_t){" LDY", NES_ADDR_MODE_ABS},
    [0xBC] = (nes_cpu_debug_op_info_t){" LDY", NES_ADDR_MODE_ABX},
    [0x81] = (nes_cpu_debug_op_info_t){" STA", NES_ADDR_MODE_NDX},
    [0x85] = (nes_cpu_debug_op_info_t){" STA", NES_ADDR_MODE_ZPG},
    [0x8D] = (nes_cpu_debug_op_info_t){" STA", NES_ADDR_MODE_ABS},
    [0x91] = (nes_cpu_debug_op_info_t){" STA", NES_ADDR_MODE_NDY},
    [0x95] = (nes_cpu_debug_op_info_t){" STA", NES_ADDR_MODE_ZPX},
    [0x99] = (nes_cpu_debug_op_info_t){" STA", NES_ADDR_MODE_ABY},
    [0x9D] = (nes_cpu_debug_op_info_t){" STA", NES_ADDR_MODE_ABX},
    [0x86] = (nes_cpu_debug_op_info_t){" STX", NES_ADDR_MODE_ZPG},
    [0x8E] = (nes_cpu_debug_op_info_t){" STX", NES_ADDR_MODE_ABS},
    [0x96] = (nes_cpu_debug_op_info_t){" STX", NES_ADDR_MODE_ZPY},
    [0x84] = (nes_cpu_debug_op_info_t){" STY", NES_ADDR_MODE_ZPG},
    [0x8C] = (nes_cpu_debug_op_info_t){" STY", NES_ADDR_MODE_ABS},
    [0x94] = (nes_cpu_debug_op_info_t){" STY", NES_ADDR_MODE_ZPX},
    [0x69] = (nes_cpu_debug_op_info_t){" ADC", NES_ADDR_MODE_IMM},
    [0x65] = (nes_cpu_debug_op_info_t){" ADC", NES_ADDR_MODE_ZPG},
    [0x75] = (nes_cpu_debug_op_info_t){" ADC", NES_ADDR_MODE_ZPX},
    [0x6D] = (nes_cpu_debug_op_info_t){" ADC", NES_ADDR_MODE_ABS},
    [0x7D] = (nes_cpu_debug_op_info_t){" ADC", NES_ADDR_MODE_ABX},
    [0x79] = (nes_cpu_debug_op_info_t){" ADC", NES_ADDR_MODE_ABY},
    [0x61] = (nes_cpu_debug_op_info_t){" ADC", NES_ADDR_MODE_NDX},
    [0x71] = (nes_cpu_debug_op_info_t){" ADC", NES_ADDR_MODE_NDY},
    [0xE9] = (nes_cpu_debug_op_info_t){" SBC", NES_ADDR_MODE_IMM},
    [0xE5] = (nes_cpu_debug_op_info_t){" SBC", NES_ADDR_MODE_ZPG},
    [0xF5] = (nes_cpu_debug_op_info_t){" SBC", NES_ADDR_MODE_ZPX},
    [0xED] = (nes_cpu_debug_op_info_t){" SBC", NES_ADDR_MODE_ABS},
    [0xFD] = (nes_cpu_debug_op_info_t){" SBC", NES_ADDR_MODE_ABX},
    [0xF9] = (nes_cpu_debug_op_info_t){" SBC", NES_ADDR_MODE_ABY},
    [0xE1] = (nes_cpu_debug_op_info_t){" SBC", NES_ADDR_MODE_NDX},
    [0xF1] = (nes_cpu_debug_op_info_t){" SBC", NES_ADDR_MODE_NDY},
    [0xC9] = (nes_cpu_debug_op_info_t){" CMP", NES_ADDR_MODE_IMM},
    [0xC5] = (nes_cpu_debug_op_info_t){" CMP", NES_ADDR_MODE_ZPG},
    [0xD5] = (nes_cpu_debug_op_info_t){" CMP", NES_ADDR_MODE_ZPX},
    [0xCD] = (nes_cpu_debug_op_info_t){" CMP", NES_ADDR_MODE_ABS},
    [0xDD] = (nes_cpu_debug_op_info_t){" CMP", NES_ADDR_MODE_ABX},
    [0xD9] = (nes_cpu_debug_op_info_t){" CMP", NES_ADDR_MODE_ABY},
    [0xC1] = (nes_cpu_debug_op_info_t){" CMP", NES_ADDR_MODE_NDX},
    [0xD1] = (nes_cpu_debug_op_info_t){" CMP", NES_ADDR_MODE_NDY},
    [0xE0] = (nes_cpu_debug_op_info_t){" CPX", NES_ADDR_MODE_IMM},
    [0xE4] = (nes_cpu_debug_op_info_t){" CPX", NES_ADDR_MODE_ZPG},
    [0xEC] = (nes_cpu_debug_op_info_t){" CPX", NES_ADDR_MODE_ABS},
    [0xC0] = (nes_cpu_debug_op_info_t){" CPY", NES_ADDR_MODE_IMM},
    [0xC4] = (nes_cpu_debug_op_info_t){" CPY", NES_ADDR_MODE_ZPG},
    [0xCC] = (nes_cpu_debug_op_info_t){" CPY", NES_ADDR_MODE_ABS},
    [0x29] = (nes_cpu_debug_op_info_t){" AND", NES_ADDR_MODE_IMM},
    [0x25] = (nes_cpu_debug_op_info_t){" AND", NES_ADDR_MODE_ZPG},
    [0x35] = (nes_cpu_debug_op_info_t){" AND", NES_ADDR_MODE_ZPX},
    [0x2D] = (nes_cpu_debug_op_info_t){" AND", NES_ADDR_MODE_ABS},
    [0x3D] = (nes_cpu_debug_op_info_t){" AND", NES_ADDR_MODE_ABX},
    [0x39] = (nes_cpu_debug_op_info_t){" AND", NES_ADDR_MODE_ABY},
    [0x21] = (nes_cpu_debug_op_info_t){" AND", NES_ADDR_MODE_NDX},
    [0x31] = (nes_cpu_debug_op_info_t){" AND", NES_ADDR_MODE_NDY},
    [0x09] = (nes_cpu_debug_op_info_t){" ORA", NES_ADDR_MODE_IMM},
    [0x05] = (nes_cpu_debug_op_info_t){" ORA", NES_ADDR_MODE_ZPG},
    [0x15] = (nes_cpu_debug_op_info_t){" ORA", NES_ADDR_MODE_ZPX},
    [0x0D] = (nes_cpu_debug_op_info_t){" ORA", NES_ADDR_MODE_ABS},
    [0x1D] = (nes_cpu_debug_op_info_t){" ORA", NES_ADDR_MODE_ABX},
    [0x19] = (nes_cpu_debug_op_info_t){" ORA", NES_ADDR_MODE_ABY},
    [0x01] = (nes_cpu_debug_op_info_t){" ORA", NES_ADDR_MODE_NDX},
    [0x11] = (nes_cpu_debug_op_info_t){" ORA", NES_ADDR_MODE_NDY},
    [0x49] = (nes_cpu_debug_op_info_t){" EOR", NES_ADDR_MODE_IMM},
    [0x45] = (nes_cpu_debug_op_info_t){" EOR", NES_ADDR_MODE_ZPG},
    [0x55] = (nes_cpu_debug_op_info_t){" EOR", NES_ADDR_MODE_ZPX},
    [0x4D] = (nes_cpu_debug_op_info_t){" EOR", NES_ADDR_MODE_ABS},
    [0x5D] = (nes_cpu_debug_op_info_t){" EOR", NES_ADDR_MODE_ABX},
    [0x59] = (nes_cpu_debug_op_info_t){" EOR", NES_ADDR_MODE_ABY},
    [0x41] = (nes_cpu_debug_op_info_t){" EOR", NES_ADDR_MODE_NDX},
    [0x51] = (nes_cpu_debug_op_info_t){" EOR", NES_ADDR_MODE_NDY},
    [0x24] = (nes_cpu_debug_op_info_t){" BIT", NES_ADDR_MODE_ZPG},
    [0x2C] = (nes_cpu_debug_op_info_t){" BIT", NES_ADDR_MODE_ABS},
    [0x2A] = (nes_cpu_debug_op_info_t){" ROL", NES_ADDR_MODE_ACC},
    [0x26] = (nes_cpu_debug_op_info_t){" ROL", NES_ADDR_MODE_ZPG},
    [0x36] = (nes_cpu_debug_op_info_t){" ROL", NES_ADDR_MODE_ZPX},
    [0x2E] = (nes_cpu_debug_op_info_t){" ROL", NES_ADDR_MODE_ABS},
    [0x3E] = (nes_cpu_debug_op_info_t){" ROL", NES_ADDR_MODE_ABX},
    [0x6A] = (nes_cpu_debug_op_info_t){" ROR", NES_ADDR_MODE_ACC},
    [0x66] = (nes_cpu_debug_op_info_t){" ROR", NES_ADDR_MODE_ZPG},
    [0x76] = (nes_cpu_debug_op_info_t){" ROR", NES_ADDR_MODE_ZPX},
    [0x6E] = (nes_cpu_debug_op_info_t){" ROR", NES_ADDR_MODE_ABS},
    [0x7E] = (nes_cpu_debug_op_info_t){" ROR", NES_ADDR_MODE_ABX},
    [0x0A] = (nes_cpu_debug_op_info_t){" ASL", NES_ADDR_MODE_ACC},
    [0x06] = (nes_cpu_debug_op_info_t){" ASL", NES_ADDR_MODE_ZPG},
    [0x16] = (nes_cpu_debug_op_info_t){" ASL", NES_ADDR_MODE_ZPX},
    [0x0E] = (nes_cpu_debug_op_info_t){" ASL", NES_ADDR_MODE_ABS},
    [0x1E] = (nes_cpu_debug_op_info_t){" ASL", NES_ADDR_MODE_ABX},
    [0x4A] = (nes_cpu_debug_op_info_t){" LSR", NES_ADDR_MODE_ACC},
    [0x46] = (nes_cpu_debug_op_info_t){" LSR", NES_ADDR_MODE_ZPG},
    [0x56] = (nes_cpu_debug_op_info_t){" LSR", NES_ADDR_MODE_ZPX},
    [0x4E] = (nes_cpu_debug_op_info_t){" LSR", NES_ADDR_MODE_ABS},
    [0x5E] = (nes_cpu_debug_op_info_t){" LSR", NES_ADDR_MODE_ABX},
    [0xE6] = (nes_cpu_debug_op_info_t){" INC", NES_ADDR_MODE_ZPG},
    [0xF6] = (nes_cpu_debug_op_info_t){" INC", NES_ADDR_MODE_ZPX},
    [0xEE] = (nes_cpu_debug_op_info_t){" INC", NES_ADDR_MODE_ABS},
    [0xFE] = (nes_cpu_debug_op_info_t){" INC", NES_ADDR_MODE_ABX},
    [0xC6] = (nes_cpu_debug_op_info_t){" DEC", NES_ADDR_MODE_ZPG},
    [0xD6] = (nes_cpu_debug_op_info_t){" DEC", NES_ADDR_MODE_ZPX},
    [0xCE] = (nes_cpu_debug_op_info_t){" DEC", NES_ADDR_MODE_ABS},
    [0xDE] = (nes_cpu_debug_op_info_t){" DEC", NES_ADDR_MODE_ABX},
    [0xE8] = (nes_cpu_debug_op_info_t){" INX", NES_ADDR_MODE_IND},
    [0xCA] = (nes_cpu_debug_op_info_t){" DEX", NES_ADDR_MODE_IND},
    [0xC8] = (nes_cpu_debug_op_info_t){" INY", NES_ADDR_MODE_IND},
    [0x88] = (nes_cpu_debug_op_info_t){" DEY", NES_ADDR_MODE_IND},
    [0xAA] = (nes_cpu_debug_op_info_t){" TAX", NES_ADDR_MODE_IND},
    [0xA8] = (nes_cpu_debug_op_info_t){" TAY", NES_ADDR_MODE_IND},
    [0x8A] = (nes_cpu_debug_op_info_t){" TXA", NES_ADDR_MODE_IND},
    [0x98] = (nes_cpu_debug_op_info_t){" TYA", NES_ADDR_MODE_IND},
    [0x9A] = (nes_cpu_debug_op_info_t){" TXS", NES_ADDR_MODE_IND},
    [0xBA] = (nes_cpu_debug_op_info_t){" TSX", NES_ADDR_MODE_IND},
    [0x18] = (nes_cpu_debug_op_info_t){" CLC", NES_ADDR_MODE_IND},
    [0x38] = (nes_cpu_debug_op_info_t){" SEC", NES_ADDR_MODE_IND},
    [0x58] = (nes_cpu_debug_op_info_t){" CLI", NES_ADDR_MODE_IND},
    [0x78] = (nes_cpu_debug_op_info_t){" SEI", NES_ADDR_MODE_IND},
    [0xB8] = (nes_cpu_debug_op_info_t){" CLV", NES_ADDR_MODE_IND},
    [0xD8] = (nes_cpu_debug_op_info_t){" CLD", NES_ADDR_MODE_IND},
    [0xF8] = (nes_cpu_debug_op_info_t){" SED", NES_ADDR_MODE_IND},
    [0x10] = (nes_cpu_debug_op_info_t){" BPL", NES_ADDR_MODE_REL},
    [0x30] = (nes_cpu_debug_op_info_t){" BMI", NES_ADDR_MODE_REL},
    [0x50] = (nes_cpu_debug_op_info_t){" BVC", NES_ADDR_MODE_REL},
    [0x70] = (nes_cpu_debug_op_info_t){" BVS", NES_ADDR_MODE_REL},
    [0x90] = (nes_cpu_debug_op_info_t){" BCC", NES_ADDR_MODE_REL},
    [0xB0] = (nes_cpu_debug_op_info_t){" BCS", NES_ADDR_MODE_REL},
    [0xD0] = (nes_cpu_debug_op_info_t){" BNE", NES_ADDR_MODE_REL},
    [0xF0] = (nes_cpu_debug_op_info_t){" BEQ", NES_ADDR_MODE_REL},
    [0x4C] = (nes_cpu_debug_op_info_t){" JMP", NES_ADDR_MODE_IND},
    [0x6C] = (nes_cpu_debug_op_info_t){" JMP", NES_ADDR_MODE_IND},
    [0x20] = (nes_cpu_debug_op_info_t){" JSR", NES_ADDR_MODE_IND},
    [0x60] = (nes_cpu_debug_op_info_t){" RTS", NES_ADDR_MODE_IND},
    [0x00] = (nes_cpu_debug_op_info_t){" BRK", NES_ADDR_MODE_IND},
    [0x40] = (nes_cpu_debug_op_info_t){" RTI", NES_ADDR_MODE_IND},
    [0x48] = (nes_cpu_debug_op_info_t){" PHA", NES_ADDR_MODE_IND},
    [0x68] = (nes_cpu_debug_op_info_t){" PLA", NES_ADDR_MODE_IND},
    [0x08] = (nes_cpu_debug_op_info_t){" PHP", NES_ADDR_MODE_IND},
    [0x28] = (nes_cpu_debug_op_info_t){" PLP", NES_ADDR_MODE_IND},
    [0xEA] = (nes_cpu_debug_op_info_t){" NOP", NES_ADDR_MODE_IND},
    [0x1A] = (nes_cpu_debug_op_info_t){"*NOP", NES_ADDR_MODE_IND},
    [0x3A] = (nes_cpu_debug_op_info_t){"*NOP", NES_ADDR_MODE_IND},
    [0x5A] = (nes_cpu_debug_op_info_t){"*NOP", NES_ADDR_MODE_IND},
    [0x7A] = (nes_cpu_debug_op_info_t){"*NOP", NES_ADDR_MODE_IND},
    [0xDA] = (nes_cpu_debug_op_info_t){"*NOP", NES_ADDR_MODE_IND},
    [0xFA] = (nes_cpu_debug_op_info_t){"*NOP", NES_ADDR_MODE_IND},
    [0x0B] = (nes_cpu_debug_op_info_t){"*ANC", NES_ADDR_MODE_IMM},
    [0x4B] = (nes_cpu_debug_op_info_t){"*ALR", NES_ADDR_MODE_IMM},
    [0x6B] = (nes_cpu_debug_op_info_t){"*ARR", NES_ADDR_MODE_IMM},
    [0xCB] = (nes_cpu_debug_op_info_t){"*AXS", NES_ADDR_MODE_IMM},
    [0xA3] = (nes_cpu_debug_op_info_t){"*LAX", NES_ADDR_MODE_NDX},
    [0xA7] = (nes_cpu_debug_op_info_t){"*LAX", NES_ADDR_MODE_ZPG},
    [0xAF] = (nes_cpu_debug_op_info_t){"*LAX", NES_ADDR_MODE_ABS},
    [0xB3] = (nes_cpu_debug_op_info_t){"*LAX", NES_ADDR_MODE_NDY},
    [0xB7] = (nes_cpu_debug_op_info_t){"*LAX", NES_ADDR_MODE_ZPY},
    [0xBF] = (nes_cpu_debug_op_info_t){"*LAX", NES_ADDR_MODE_ABY},
    [0x83] = (nes_cpu_debug_op_info_t){"*SAX", NES_ADDR_MODE_NDX},
    [0x87] = (nes_cpu_debug_op_info_t){"*SAX", NES_ADDR_MODE_ZPG},
    [0x8F] = (nes_cpu_debug_op_info_t){"*SAX", NES_ADDR_MODE_ABS},
    [0x97] = (nes_cpu_debug_op_info_t){"*SAX", NES_ADDR_MODE_ZPY},
    [0xC3] = (nes_cpu_debug_op_info_t){"*DCP", NES_ADDR_MODE_NDX},
    [0xC7] = (nes_cpu_debug_op_info_t){"*DCP", NES_ADDR_MODE_ZPG},
    [0xCF] = (nes_cpu_debug_op_info_t){"*DCP", NES_ADDR_MODE_ABS},
    [0xD3] = (nes_cpu_debug_op_info_t){"*DCP", NES_ADDR_MODE_NDY},
    [0xD7] = (nes_cpu_debug_op_info_t){"*DCP", NES_ADDR_MODE_ZPX},
    [0xDB] = (nes_cpu_debug_op_info_t){"*DCP", NES_ADDR_MODE_ABY},
    [0xDF] = (nes_cpu_debug_op_info_t){"*DCP", NES_ADDR_MODE_ABX},
    [0xE3] = (nes_cpu_debug_op_info_t){"*ISB", NES_ADDR_MODE_NDX},
    [0xE7] = (nes_cpu_debug_op_info_t){"*ISB", NES_ADDR_MODE_ZPG},
    [0xEF] = (nes_cpu_debug_op_info_t){"*ISB", NES_ADDR_MODE_ABS},
    [0xF3] = (nes_cpu_debug_op_info_t){"*ISB", NES_ADDR_MODE_NDY},
    [0xF7] = (nes_cpu_debug_op_info_t){"*ISB", NES_ADDR_MODE_ZPX},
    [0xFB] = (nes_cpu_debug_op_info_t){"*ISB", NES_ADDR_MODE_ABY},
    [0xFF] = (nes_cpu_debug_op_info_t){"*ISB", NES_ADDR_MODE_ABX},
    [0x23] = (nes_cpu_debug_op_info_t){"*RLA", NES_ADDR_MODE_NDX},
    [0x27] = (nes_cpu_debug_op_info_t){"*RLA", NES_ADDR_MODE_ZPG},
    [0x2F] = (nes_cpu_debug_op_info_t){"*RLA", NES_ADDR_MODE_ABS},
    [0x33] = (nes_cpu_debug_op_info_t){"*RLA", NES_ADDR_MODE_NDY},
    [0x37] = (nes_cpu_debug_op_info_t){"*RLA", NES_ADDR_MODE_ZPX},
    [0x3B] = (nes_cpu_debug_op_info_t){"*RLA", NES_ADDR_MODE_ABY},
    [0x3F] = (nes_cpu_debug_op_info_t){"*RLA", NES_ADDR_MODE_ABX},
    [0x63] = (nes_cpu_debug_op_info_t){"*RRA", NES_ADDR_MODE_NDX},
    [0x67] = (nes_cpu_debug_op_info_t){"*RRA", NES_ADDR_MODE_ZPG},
    [0x6F] = (nes_cpu_debug_op_info_t){"*RRA", NES_ADDR_MODE_ABS},
    [0x73] = (nes_cpu_debug_op_info_t){"*RRA", NES_ADDR_MODE_NDY},
    [0x77] = (nes_cpu_debug_op_info_t){"*RRA", NES_ADDR_MODE_ZPX},
    [0x7B] = (nes_cpu_debug_op_info_t){"*RRA", NES_ADDR_MODE_ABY},
    [0x7F] = (nes_cpu_debug_op_info_t){"*RRA", NES_ADDR_MODE_ABX},
    [0x03] = (nes_cpu_debug_op_info_t){"*SLO", NES_ADDR_MODE_NDX},
    [0x07] = (nes_cpu_debug_op_info_t){"*SLO", NES_ADDR_MODE_ZPG},
    [0x0F] = (nes_cpu_debug_op_info_t){"*SLO", NES_ADDR_MODE_ABS},
    [0x13] = (nes_cpu_debug_op_info_t){"*SLO", NES_ADDR_MODE_NDY},
    [0x17] = (nes_cpu_debug_op_info_t){"*SLO", NES_ADDR_MODE_ZPX},
    [0x1B] = (nes_cpu_debug_op_info_t){"*SLO", NES_ADDR_MODE_ABY},
    [0x1F] = (nes_cpu_debug_op_info_t){"*SLO", NES_ADDR_MODE_ABX},
    [0x43] = (nes_cpu_debug_op_info_t){"*SRE", NES_ADDR_MODE_NDX},
    [0x47] = (nes_cpu_debug_op_info_t){"*SRE", NES_ADDR_MODE_ZPG},
    [0x4F] = (nes_cpu_debug_op_info_t){"*SRE", NES_ADDR_MODE_ABS},
    [0x53] = (nes_cpu_debug_op_info_t){"*SRE", NES_ADDR_MODE_NDY},
    [0x57] = (nes_cpu_debug_op_info_t){"*SRE", NES_ADDR_MODE_ZPX},
    [0x5B] = (nes_cpu_debug_op_info_t){"*SRE", NES_ADDR_MODE_ABY},
    [0x5F] = (nes_cpu_debug_op_info_t){"*SRE", NES_ADDR_MODE_ABX},
    [0xEB] = (nes_cpu_debug_op_info_t){"*SBC", NES_ADDR_MODE_IMM},
    [0x80] = (nes_cpu_debug_op_info_t){"*NOP", NES_ADDR_MODE_IMM},
    [0x82] = (nes_cpu_debug_op_info_t){"*NOP", NES_ADDR_MODE_IMM},
    [0x89] = (nes_cpu_debug_op_info_t){"*NOP", NES_ADDR_MODE_IMM},
    [0xC2] = (nes_cpu_debug_op_info_t){"*NOP", NES_ADDR_MODE_IMM},
    [0xE2] = (nes_cpu_debug_op_info_t){"*NOP", NES_ADDR_MODE_IMM},
    [0x0C] = (nes_cpu_debug_op_info_t){"*NOP", NES_ADDR_MODE_ABS},
    [0x1C] = (nes_cpu_debug_op_info_t){"*NOP", NES_ADDR_MODE_ABX},
    [0x3C] = (nes_cpu_debug_op_info_t){"*NOP", NES_ADDR_MODE_ABX},
    [0x5C] = (nes_cpu_debug_op_info_t){"*NOP", NES_ADDR_MODE_ABX},
    [0x7C] = (nes_cpu_debug_op_info_t){"*NOP", NES_ADDR_MODE_ABX},
    [0xDC] = (nes_cpu_debug_op_info_t){"*NOP", NES_ADDR_MODE_ABX},
    [0xFC] = (nes_cpu_debug_op_info_t){"*NOP", NES_ADDR_MODE_ABX},
    [0x04] = (nes_cpu_debug_op_info_t){"*NOP", NES_ADDR_MODE_ZPG},
    [0x44] = (nes_cpu_debug_op_info_t){"*NOP", NES_ADDR_MODE_ZPG},
    [0x64] = (nes_cpu_debug_op_info_t){"*NOP", NES_ADDR_MODE_ZPG},
    [0x14] = (nes_cpu_debug_op_info_t){"*NOP", NES_ADDR_MODE_ZPX},
    [0x34] = (nes_cpu_debug_op_info_t){"*NOP", NES_ADDR_MODE_ZPX},
    [0x54] = (nes_cpu_debug_op_info_t){"*NOP", NES_ADDR_MODE_ZPX},
    [0x74] = (nes_cpu_debug_op_info_t){"*NOP", NES_ADDR_MODE_ZPX},
    [0xD4] = (nes_cpu_debug_op_info_t){"*NOP", NES_ADDR_MODE_ZPX},
    [0xF4] = (nes_cpu_debug_op_info_t){"*NOP", NES_ADDR_MODE_ZPX},
  };

  // preserve PPU registers, some break on read
  uint8_t ppu_flags = nes->ppu.flags;
  uint8_t ppu_status = nes->ppu.status;
  uint8_t ppu_mask = nes->ppu.mask;
  uint8_t ppu_ctrl = nes->ppu.ctrl;
  uint16_t ppu_vmem_addr = nes->ppu.vmem_addr;
  uint16_t ppu_tmp_addr = nes->ppu.tmp_addr;
  uint8_t ppu_oam_addr = nes->ppu.oam_addr;

  uint8_t opcode = nes_mem_readb(nes, nes->cpu.pc);

  fprintf(stream, "%02X ", opcode);

  if (!op_info[opcode].txt) {
    fprintf(stream, "       UNDEFINED                       ");
    goto cleanup;
  }

  uint16_t addr;
  switch (op_info[opcode].mode) {
    case NES_ADDR_MODE_ABS:
      addr = nes_mem_readw(nes, nes->cpu.pc + 1);
      fprintf(stream, "%02X %02X %s $%04X = %02X                  ",
              nes_mem_readb(nes, nes->cpu.pc + 1),
              nes_mem_readb(nes, nes->cpu.pc + 2), op_info[opcode].txt, addr,
              nes_mem_readb(nes, addr));
      goto cleanup;
    case NES_ADDR_MODE_ABX:
      addr = nes_mem_readw(nes, nes->cpu.pc + 1) + nes->cpu.x;
      fprintf(stream, "%02X %02X %s $%04X,X @ %04X = %02X         ",
              nes_mem_readb(nes, nes->cpu.pc + 1),
              nes_mem_readb(nes, nes->cpu.pc + 2), op_info[opcode].txt,
              nes_mem_readw(nes, nes->cpu.pc + 1), addr,
              nes_mem_readb(nes, addr));
      goto cleanup;
    case NES_ADDR_MODE_ABY:
      addr = nes_mem_readw(nes, nes->cpu.pc + 1) + nes->cpu.y;
      fprintf(stream, "%02X %02X %s $%04X,Y @ %04X = %02X         ",
              nes_mem_readb(nes, nes->cpu.pc + 1),
              nes_mem_readb(nes, nes->cpu.pc + 2), op_info[opcode].txt,
              nes_mem_readw(nes, nes->cpu.pc + 1), addr,
              nes_mem_readb(nes, addr));
      goto cleanup;
    case NES_ADDR_MODE_ACC:
      fprintf(stream, "      %s A                           ",
              op_info[opcode].txt);
      goto cleanup;
    case NES_ADDR_MODE_IMM:
      fprintf(stream, "%02X    %s #$%02X                        ",
              nes_mem_readb(nes, nes->cpu.pc + 1), op_info[opcode].txt,
              nes_mem_readb(nes, nes->cpu.pc + 1));
      goto cleanup;
    case NES_ADDR_MODE_NDX:
      addr =
        nes_mem_readw_zp(nes, nes_mem_readb(nes, nes->cpu.pc + 1) + nes->cpu.x);
      fprintf(stream, "%02X    %s ($%02X,X) @ %02X = %04X = %02X    ",
              nes_mem_readb(nes, nes->cpu.pc + 1), op_info[opcode].txt,
              nes_mem_readb(nes, nes->cpu.pc + 1),
              (nes_mem_readb(nes, nes->cpu.pc + 1) + nes->cpu.x) & 0xFF, addr,
              nes_mem_readb(nes, addr));
      goto cleanup;
    case NES_ADDR_MODE_NDY:
      addr =
        nes_mem_readw_zp(nes, nes_mem_readb(nes, nes->cpu.pc + 1)) + nes->cpu.y;
      fprintf(stream, "%02X    %s ($%02X),Y = %04X @ %04X = %02X  ",
              nes_mem_readb(nes, nes->cpu.pc + 1), op_info[opcode].txt,
              nes_mem_readb(nes, nes->cpu.pc + 1),
              nes_mem_readw_zp(nes, nes_mem_readb(nes, nes->cpu.pc + 1)), addr,
              nes_mem_readb(nes, addr));
      goto cleanup;
    case NES_ADDR_MODE_REL:
      addr = nes->cpu.pc + (int8_t)nes_mem_readb(nes, nes->cpu.pc + 1) + 2;
      fprintf(stream, "%02X    %s $%04X                       ",
              nes_mem_readb(nes, nes->cpu.pc + 1), op_info[opcode].txt, addr);
      goto cleanup;
    case NES_ADDR_MODE_ZPG:
      addr = nes_mem_readb(nes, nes->cpu.pc + 1);
      fprintf(stream, "%02X    %s $%02X = %02X                    ",
              nes_mem_readb(nes, nes->cpu.pc + 1), op_info[opcode].txt, addr,
              nes_mem_readb_zp(nes, addr));
      goto cleanup;
    case NES_ADDR_MODE_ZPX:
      addr = nes_mem_readb(nes, nes->cpu.pc + 1) + nes->cpu.x;
      fprintf(stream, "%02X    %s $%02X,X @ %02X = %02X             ",
              nes_mem_readb(nes, nes->cpu.pc + 1), op_info[opcode].txt,
              nes_mem_readb(nes, nes->cpu.pc + 1), addr & 0xFF,
              nes_mem_readb_zp(nes, addr));
      goto cleanup;
    case NES_ADDR_MODE_ZPY:
      addr = nes_mem_readb(nes, nes->cpu.pc + 1) + nes->cpu.y;
      fprintf(stream, "%02X    %s $%02X,Y @ %02X = %02X             ",
              nes_mem_readb(nes, nes->cpu.pc + 1), op_info[opcode].txt,
              nes_mem_readb(nes, nes->cpu.pc + 1), addr & 0xFF,
              nes_mem_readb_zp(nes, addr));
      goto cleanup;
  }

  if ((opcode == 0x4C) || (opcode == 0x20)) {
    addr = nes_mem_readw(nes, nes->cpu.pc + 1);
    fprintf(stream, "%02X %02X %s $%04X                       ",
            nes_mem_readb(nes, nes->cpu.pc + 1),
            nes_mem_readb(nes, nes->cpu.pc + 2), op_info[opcode].txt, addr);

    goto cleanup;
  }

  if (opcode == 0x6C) {
    addr = nes_mem_readw(nes, nes->cpu.pc + 1);
    fprintf(stream, "%02X %02X %s ($%04X) = %04X              ",
            nes_mem_readb(nes, nes->cpu.pc + 1),
            nes_mem_readb(nes, nes->cpu.pc + 2), op_info[opcode].txt, addr,
            nes_jmi_addr(nes, addr));

    goto cleanup;
  }

  fprintf(stream, "      %s                             ",
          op_info[opcode].txt);

cleanup:
  // restore PPU registers
  nes->ppu.flags = ppu_flags;
  nes->ppu.status = ppu_status;
  nes->ppu.mask = ppu_mask;
  nes->ppu.ctrl = ppu_ctrl;
  nes->ppu.vmem_addr = ppu_vmem_addr;
  nes->ppu.tmp_addr = ppu_tmp_addr;
  nes->ppu.oam_addr = ppu_oam_addr;
}

static inline void nes_cpu_debug_print_axy_full(nes_t *nes, FILE *stream) {
  fprintf(stream, "A:%02X X:%02X Y:%02X ", nes->cpu.a, nes->cpu.x, nes->cpu.y);
}

static inline void nes_cpu_debug_print_flags_short(nes_t *nes, FILE *stream) {
  fprintf(stream, "P:%02X ", nes->cpu.p);
}

static inline void nes_cpu_debug_print_flags_full(nes_t *nes, FILE *stream) {
  fprintf(stream, "P:%c%c1%c%c%c%c%c ",
          (BITMGET(nes->cpu.p, FLAG_MASK_N)) ? 'N' : '-',
          (BITMGET(nes->cpu.p, FLAG_MASK_V)) ? 'V' : '-',
          (BITMGET(nes->cpu.p, FLAG_MASK_B)) ? 'B' : '-',
          (BITMGET(nes->cpu.p, FLAG_MASK_D)) ? 'D' : '-',
          (BITMGET(nes->cpu.p, FLAG_MASK_I)) ? 'I' : '-',
          (BITMGET(nes->cpu.p, FLAG_MASK_Z)) ? 'Z' : '-',
          (BITMGET(nes->cpu.p, FLAG_MASK_C)) ? 'C' : '-');
}

static inline void nes_cpu_debug_print_stack_med(nes_t *nes, FILE *stream) {
  fprintf(stream, "SP:%02X\n", nes->cpu.s);
}

static inline void nes_cpu_debug_print_ctx_full(nes_t *nes, FILE *stream) {
  nes_cpu_debug_print_axy_full(nes, stream);
  nes_cpu_debug_print_flags_full(nes, stream);
  nes_cpu_debug_print_stack_med(nes, stream);
}

static inline void nes_cpu_debug_print_ctx_nesulator(nes_t *nes, FILE *stream) {
  nes_cpu_debug_print_axy_full(nes, stream);
  nes_cpu_debug_print_flags_short(nes, stream);
  nes_cpu_debug_print_stack_med(nes, stream);
}

static inline void nes_cpu_debug_print_full(nes_t *nes, FILE *stream) {
  fprintf(stream, "%04X  ", nes->cpu.pc);
  nes_cpu_debug_print_op_full(nes, stream);
  nes_cpu_debug_print_ctx_full(nes, stream);
}

static inline void nes_cpu_debug_print_nesulator(nes_t *nes, FILE *stream) {
  fprintf(stream, "%04X  ", nes->cpu.pc);
  nes_cpu_debug_print_op_full(nes, stream);
  nes_cpu_debug_print_ctx_nesulator(nes, stream);
}