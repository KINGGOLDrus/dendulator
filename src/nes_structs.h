#pragma once

#include <stdint.h>

// CPU state struct
typedef struct {
  uint64_t cycle; // cycle counter
  uint64_t stall; // stall cycle counter ("wait for this many cycles")
  uint8_t pages_crossed; // >0 when a page boundary was crossed on last rw op

  // registers
  uint16_t pc; // program counter
  uint8_t a; // accumulator
  uint8_t x; // index register
  uint8_t y; // index register
  uint8_t s; // stack pointer
  uint8_t p; // flags
} nes_cpu_t;

// APU square channel state struct
typedef struct {
  uint8_t flags; // state flags

  uint8_t chan; // which channel is this for
  uint8_t length; // signal length

  uint16_t tmr_period;
  uint16_t tmr_val;

  uint8_t duty_mode;
  uint8_t duty_val;

  uint8_t sweep_shift;
  uint8_t sweep_period;
  int8_t sweep_val;

  int8_t env_val;
  uint8_t env_period;
  uint8_t env_vol;

  uint8_t const_vol;
} nes_apu_sqr_t;

// APU triangle channel state struct
typedef struct {
  uint8_t flags; // state flags

  uint8_t length; // signal length

  uint16_t tmr_period;
  int16_t tmr_val;

  uint8_t duty_val;
  uint8_t duty_out;

  uint8_t counter_period;
  int8_t counter_val;
} nes_apu_tri_t;

// APU noise channel state struct
typedef struct {
  uint8_t flags; // state flags

  uint16_t shift; // timer shift
  uint8_t length; // signal length

  uint16_t tmr_period;
  uint16_t tmr_val;

  int8_t env_val;
  uint8_t env_period;
  uint8_t env_vol;

  uint8_t const_vol;
} nes_apu_noi_t;

// APU DMC channel state struct
typedef struct {
  uint8_t flags; // state flags

  uint8_t value; // current signal value

  uint16_t sample_addr;
  uint16_t sample_length;

  uint16_t cur_addr;
  uint16_t cur_length;

  uint8_t shift;
  uint8_t bit;

  uint8_t tick_period;
  uint8_t tick_value;
} nes_apu_dmc_t;

// APU state struct
typedef struct {
  uint64_t cycle; // cycle counter

  // channels
  nes_apu_sqr_t sq1;
  nes_apu_sqr_t sq2;
  nes_apu_tri_t tri;
  nes_apu_noi_t noi;
  nes_apu_dmc_t dmc;

  uint8_t frame_period;
  uint8_t frame_val;
  uint8_t frame_irq;

  // output buffer
  uint8_t *buf;
  uint32_t buf_size;
  uint32_t max_buf_size;
} nes_apu_t;

// RAM/ROM state struct
typedef struct {
  uint8_t ram[0x800]; // RAM
  uint8_t prgram[0x2000]; // PRG-RAM
  uint8_t *prg[2]; // current PRG-ROM banks
} nes_mem_t;

// PPU tile data
typedef struct {
  uint8_t nta;
  uint8_t attr;
  uint8_t data_lo;
  uint8_t data_hi;
  uint64_t data;
} nes_ppu_tile_t;

// PPU sprite data
typedef struct {
  uint32_t data;
  uint8_t pos;
  uint8_t pri;
  uint8_t idx;
} nes_ppu_spr_t;

// frame buffer
typedef struct {
  uint32_t data[240][256];
} nes_ppu_screen_t;

// PPU state struct
typedef struct {
  int32_t cycle; // cycle counter
  uint32_t frame; // frame counter
  int32_t scanline; // scanline counter
  uint8_t flags; // state flags

  uint8_t ctrl; // PPUCTRL
  uint8_t mask; // PPUMASK
  uint8_t status; // PPUSTATUS

  uint8_t oam_addr; // OAM DMA address
  uint16_t vmem_addr; // current video memory pointer
  uint16_t tmp_addr; // "shadow" video memory pointer
  uint8_t fine_x; // fine x scroll value

  uint8_t readb; // last byte read
  uint8_t bus; // current value on bus
  uint32_t bus_decay; // bus value decay timeout

  uint32_t nmi_delay; // cycles before NMI
  uint8_t nmi_prev; // previous NMI state
  uint8_t frame_end; // frame end flag

  uint32_t spr_count; // sprite count
  nes_ppu_tile_t tile; // current tile data
  nes_ppu_spr_t spr[8]; // sprite data for current scanline

  // frame buffers
  nes_ppu_screen_t *front;
  nes_ppu_screen_t *back;
} nes_ppu_t;

// VRAM state struct
typedef struct {
  uint8_t vram[0x1000]; // nametable data
  uint8_t oam[0x100]; // OAM data
  uint8_t pal[0x20]; // current palette
} nes_vmem_t;

// input device state struct
typedef struct {
  uint8_t btns; // button states (1 bit for each)
  uint8_t ignored; // unused
  uint8_t devid; // device id
} nes_player_input_state_t;

// player input state struct
typedef struct {
  nes_player_input_state_t cur; // current device state
  nes_player_input_state_t saved; // device state before strobe
} nes_player_input_t;

// input state struct
typedef struct {
  nes_player_input_t p1; // player 1 input
  nes_player_input_t p2; // player 2 input
  uint8_t last_write; // last write to the input register
} nes_input_t;

typedef struct nes nes_t;

// mapper interface function types
typedef void (*nes_map_init_func_t)(nes_t *nes);
typedef void (*nes_map_cleanup_func_t)(nes_t *nes);
typedef void (*nes_map_tick_func_t)(nes_t *nes); // called after each PPU tick
typedef uint8_t (*nes_read_func_t)(nes_t *nes, uint16_t addr);
typedef void (*nes_write_func_t)(nes_t *nes, uint16_t addr, uint8_t value);

// mapper interface struct
typedef struct {
  nes_map_init_func_t init; // init function pointer
  nes_map_cleanup_func_t cleanup; // cleanup function pointer
  nes_map_tick_func_t tick; // tick function pointer (can be NULL)

  nes_read_func_t read; // CPU memory read function
  nes_write_func_t write; // CPU memory write function

  nes_read_func_t vread; // PPU memory read function
  nes_write_func_t vwrite; // PPU memory write function
} nes_mapper_funcs_t;

// mapper state struct
typedef struct {
  nes_mapper_funcs_t funcs; // mapper interface

  void *extra; // extra mapper data (allocated and handled by mapper)
} nes_mapper_t;

// screen mirroring function type
// accepts PPU address, returns mirrored address
typedef uint16_t (*nes_mirror_func_t)(uint16_t addr);

// cartridge struct
typedef struct {
  nes_mapper_t mapper;
  nes_mirror_func_t mirror;

  uint8_t chr_ram; // if 1, CHR-RAM is present

  uint8_t rom16_count; // 16k PRG-ROM bank count
  uint8_t **rom; // array of 16k PRG-ROM banks

  uint8_t vram8_count; // 8k CHR bank count
  uint8_t **vram; // array of 8k CHR banks
} nes_cart_t;

// NES state struct
struct nes {
  nes_cpu_t cpu;
  nes_apu_t apu;
  nes_mem_t mem;

  nes_ppu_t ppu;
  nes_vmem_t vmem;

  nes_input_t input;
  nes_cart_t cart;
};
