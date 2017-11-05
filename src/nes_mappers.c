#include "nes_mappers.h"
#include "error.h"
#include "errcodes.h"

// add any new mappers here

#include "mappers/nrom.h"
#include "mappers/mmc1.h"
#include "mappers/unrom.h"
#include "mappers/cnrom.h"
#include "mappers/mmc3.h"

void nes_mapper_init(nes_t *nes) {
  nes->cart.mapper.funcs.init(nes);
}

void nes_mapper_tick(nes_t *nes) {
  if (nes->cart.mapper.funcs.tick)
    nes->cart.mapper.funcs.tick(nes);
}

void nes_mapper_cleanup(nes_t *nes) {
  nes->cart.mapper.funcs.cleanup(nes);
}

// mapper registry stuff

// supported mappers list; mappers add themselves to this using
// nes_reg_mapper() in module constructors
static struct mapper_info {
  uint8_t id;
  const char *name;
  nes_mapper_funcs_t *funcs;
} *nes_mappers[NES_MAX_MAPPERS] = {NULL};

void nes_get_mapper_funcs(uint8_t id, nes_mapper_funcs_t *funcs) {
  if (!nes_mappers[id]) {
    error_set_code(ERR_ROM_LOAD);
    error_log_write("Unknown or unsupported mapper!\n");
    return;
  }

  funcs->init = nes_mappers[id]->funcs->init;
  funcs->cleanup = nes_mappers[id]->funcs->cleanup;
  funcs->tick = nes_mappers[id]->funcs->tick;
  funcs->read = nes_mappers[id]->funcs->read;
  funcs->write = nes_mappers[id]->funcs->write;
  funcs->vread = nes_mappers[id]->funcs->vread;
  funcs->vwrite = nes_mappers[id]->funcs->vwrite;
}

const char *nes_get_mapper_name(uint8_t id) {
  if (nes_mappers[id])
    return nes_mappers[id]->name;
  else
    return NULL;
}

void nes_reg_mapper(uint8_t id, const char *name, nes_mapper_funcs_t *funcs) {
  if (nes_mappers[id]) {
    error_set_code(ERR_ROM_LOAD);
    error_log_write("This mapper already exists:\n");
    error_log_write(nes_mappers[id]->name);
    error_log_write("\n");
    return;
  }

  nes_mappers[id] = calloc(1, sizeof(struct mapper_info));
  nes_mappers[id]->name = name;
  nes_mappers[id]->funcs = funcs;
  nes_mappers[id]->id = id;

  printf("Registered mapper %d (%s)\n", id, name);
}

void nes_unreg_mapper(uint8_t id) {
  if (!nes_mappers[id]) {
    error_set_code(ERR_ROM_LOAD);
    error_log_write("Trying to unregister a mapper that doesn't exist!\n");
    return;
  }

  free(nes_mappers[id]);
  nes_mappers[id] = NULL;
}

uint8_t nes_supported_mapper(uint8_t id) {
  return nes_mappers[id] != NULL;
}
