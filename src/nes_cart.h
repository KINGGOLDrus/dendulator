#pragma once

#include "nes_structs.h"

// supported screen mirroring modes
enum mirror_mode {
  MIRROR_VERTICAL,
  MIRROR_HORIZONTAL,
  MIRROR_NONE,
  MIRROR_SINGLESCREEN0,
  MIRROR_SINGLESCREEN1,
  MIRROR_CUSTOM, // for funky custom mapper modes
};

void nes_cart_load(nes_t *nes, const char *fname);
void nes_cart_set_mirroring(nes_t *nes, enum mirror_mode mode);
enum mirror_mode nes_cart_get_mirroring(nes_t *nes);
void nes_cart_unload(nes_t *nes);
