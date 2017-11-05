#pragma once

#include "nes_structs.h"

void nes_apu_init(nes_apu_t *apu, uint32_t buf_size);
void nes_apu_cleanup(nes_apu_t *apu);
void nes_apu_tick(nes_t *nes);

uint8_t nes_apu_read(nes_t *nes, uint16_t addr);
void nes_apu_write(nes_t *nes, uint16_t addr, uint8_t val);
