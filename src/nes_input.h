#pragma once

#include "nes_structs.h"
#include "bitops.h"

// button bit numbers
enum nes_input_code_bit {
  NES_INPUT_BIT_A,
  NES_INPUT_BIT_B,
  NES_INPUT_BIT_SELECT,
  NES_INPUT_BIT_START,
  NES_INPUT_BIT_UP,
  NES_INPUT_BIT_DOWN,
  NES_INPUT_BIT_LEFT,
  NES_INPUT_BIT_RIGHT,
};

void nes_input_init(nes_input_t *p);
void nes_input_write(nes_t *nes, uint16_t addr, uint8_t val);
uint8_t nes_input_read(nes_t *nes, uint16_t addr);
