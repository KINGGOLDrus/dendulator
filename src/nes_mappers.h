#pragma once

#include "nes_structs.h"

// helper macro for module constructor shit
#define MAPPER_REG_FUNC __attribute__((constructor))
#define MAPPER_UNREG_FUNC __attribute__((destructor))

#define NES_MAX_MAPPERS 256

void nes_get_mapper_funcs(uint8_t id, nes_mapper_funcs_t *funcs);
const char *nes_get_mapper_name(uint8_t id);
void nes_reg_mapper(uint8_t id, const char *name, nes_mapper_funcs_t *funcs);
void nes_unreg_mapper(uint8_t id);
uint8_t nes_supported_mapper(uint8_t id);

void nes_mapper_init(nes_t *nes);
void nes_mapper_tick(nes_t *nes);
void nes_mapper_cleanup(nes_t *nes);
