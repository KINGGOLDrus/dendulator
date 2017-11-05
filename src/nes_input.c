#include "nes_input.h"

void nes_input_init(nes_input_t *p){
  p->p1.cur = (nes_player_input_state_t){0};
  p->p1.saved = (nes_player_input_state_t){0};
  p->p2.cur = (nes_player_input_state_t){0};
  p->p2.saved = (nes_player_input_state_t){0};
  p->last_write = 0x00;
}

void nes_input_write(nes_t *nes, uint16_t addr, uint8_t val){
  if (addr == 0) {
    // technically the controller state gets reloaded constantly
    // while the strobe bit is 1, but screw that
    if ((val ^ nes->input.last_write) == 0x01) {
      nes->input.p1.saved = nes->input.p1.cur;
      nes->input.p2.saved = nes->input.p2.cur;
    }
    nes->input.last_write = val;
  }
}

uint8_t nes_input_read(nes_t *nes, uint16_t addr){
  uint8_t result = 0x00;
  // shift the internal register's contents by 1 and
  // output the bit that was shifted out
  if (addr == 0) {
    result = nes->input.p1.saved.btns & 0x01;
    nes->input.p1.saved.btns >>= 1;
  } else {
    result = nes->input.p2.saved.btns & 0x01;
    nes->input.p2.saved.btns >>= 1;
  }
  return result;
}
