#include "nes_apu.h"

#include "bitops.h"
#include "nes_cpu.h"

#define NES_APU_FLAG_SQR_ENABLED 0x01
#define NES_APU_FLAG_SQR_SWEEP_RELOAD 0x02
#define NES_APU_FLAG_SQR_SWEEP_ENABLED 0x04
#define NES_APU_FLAG_SQR_SWEEP_NEGATE 0x08
#define NES_APU_FLAG_SQR_LEN_ENABLED 0x10
#define NES_APU_FLAG_SQR_ENV_ENABLED 0x20
#define NES_APU_FLAG_SQR_ENV_LOOP 0x40
#define NES_APU_FLAG_SQR_ENV_START 0x80

#define NES_APU_FLAG_TRI_ENABLED 0x01
#define NES_APU_FLAG_TRI_COUNTER_RELOAD 0x02
#define NES_APU_FLAG_TRI_LEN_ENABLED 0x10

#define NES_APU_FLAG_NOI_ENABLED 0x01
#define NES_APU_FLAG_NOI_MODE 0x02
#define NES_APU_FLAG_NOI_LEN_ENABLED 0x10
#define NES_APU_FLAG_NOI_ENV_ENABLED 0x20
#define NES_APU_FLAG_NOI_ENV_LOOP 0x40
#define NES_APU_FLAG_NOI_ENV_START 0x80

#define NES_APU_FLAG_DMC_ENABLED 0x01
#define NES_APU_FLAG_DMC_LOOP 0x02
#define NES_APU_FLAG_DMC_IRQ 0x04

static const double nes_apu_frame_counter_rate = 1789773.0 / 240.0;
static const double nes_apu_sample_rate = 1789773.0 / 48000.0;

// length and pulse tables

static uint8_t len_tbl[] = {
  10, 254, 20, 2, 40, 4, 80, 6, 160, 8, 60, 10, 14, 12, 26, 14,
  12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30,
};

static uint8_t duty_tbl[4][8] = {
  { 0, 1, 0, 0, 0, 0, 0, 0 },
  { 0, 1, 1, 0, 0, 0, 0, 0 },
  { 0, 1, 1, 1, 1, 0, 0, 0 },
  { 1, 0, 0, 1, 1, 1, 1, 1 },
};

// signal value tables

static uint8_t tri_tbl[] = {
  15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
};

static uint16_t noi_tbl[] = {
  4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068,
};

static uint8_t dmc_tbl[] = {
  214, 190, 170, 160, 143, 127, 113, 107, 95, 80, 71, 64, 53, 42, 36, 27,
};

// lookup tables for mixed signals, used in nes_apu_get_output()
static float sqr_tbl[31] = { 0 }; // for the two square channels
static float tnd_tbl[203] = { 0 }; // for the noise, triangle and DMC channels

// fills the above LUTs
static inline void nes_apu_init_tbls() {
  for (int i = 0; i < 31; ++i)
    sqr_tbl[i] = 95.52 / (8128.0 / (float)i + 100);

  for (int i = 0; i < 203; ++i)
    tnd_tbl[i] = 163.67 / (24329.0 / (float)i + 100);
}

void nes_apu_init(nes_apu_t *apu, uint32_t bsize) {
  *apu = (nes_apu_t) {
    .noi = (nes_apu_noi_t) { .shift = 1 },
    .sq1 = (nes_apu_sqr_t) { .chan = 1 },
    .sq2 = (nes_apu_sqr_t) { .chan = 2 },

    .buf = malloc(bsize * sizeof(uint8_t)),
    .max_buf_size = bsize,
    .buf_size = 0,
  };

  nes_apu_init_tbls();
}

// clears APU sample buffer
void nes_apu_cleanup(nes_apu_t *apu) {
  if (apu->buf)
    free(apu->buf);
}

// square (two channels)
// consists of envelope generator, sweep unit, timer, sequencer, length counter
// output is the envelope value, unless
// sequencer == 0, length == 0 or timer < 8
// depending on duty mode, seq. outputs one of the 3 waveforms seen in duty_tbl
// sequencer is stepped each time the timer value rolls over from 0 to T
// (the timer counts down each APU tick), where T is the timer period

// control register write
static inline void nes_apu_sqr_write_ctrl(nes_apu_sqr_t *sqr, uint8_t val) {
  sqr->duty_mode = (val >> 6) & 0x03;
  sqr->env_period = val & 0x0F;
  sqr->const_vol = val & 0x0F;
  sqr->flags = BITMCHG(sqr->flags, NES_APU_FLAG_SQR_LEN_ENABLED,
    !((val >> 5) & 0x01));
  sqr->flags = BITMCHG(sqr->flags, NES_APU_FLAG_SQR_ENV_LOOP,
    ((val >> 5) & 0x01));
  sqr->flags = BITMCHG(sqr->flags, NES_APU_FLAG_SQR_ENV_ENABLED,
    !((val >> 4) & 0x01));
  sqr->flags = BITMSET(sqr->flags, NES_APU_FLAG_SQR_ENV_START);
}

// sweep write
static inline void nes_apu_sqr_write_sweep(nes_apu_sqr_t *sqr, uint8_t val) {
  sqr->sweep_shift = val & 0x07;
  sqr->sweep_period = ((val >> 4) & 0x07) + 1;
  sqr->flags = BITMCHG(sqr->flags, NES_APU_FLAG_SQR_SWEEP_ENABLED,
    ((val >> 7) & 0x01));
  sqr->flags = BITMCHG(sqr->flags, NES_APU_FLAG_SQR_SWEEP_NEGATE,
    ((val >> 3) & 0x01));
  sqr->flags = BITMSET(sqr->flags, NES_APU_FLAG_SQR_SWEEP_RELOAD);
}

// timer lo write
static inline void nes_apu_sqr_write_tmr_low(nes_apu_sqr_t *sqr, uint8_t val) {
  sqr->tmr_period = (sqr->tmr_period & 0xFF00) | (uint16_t)(val);
}

// timer hi write
static inline void nes_apu_sqr_write_tmr_high(nes_apu_sqr_t *sqr, uint8_t val) {
  sqr->length = len_tbl[val >> 3];
  sqr->tmr_period = (sqr->tmr_period & 0x00FF) | ((uint16_t)(val & 0x07) << 8);
  sqr->flags = BITMSET(sqr->flags, NES_APU_FLAG_SQR_ENV_START);
  sqr->duty_val = 0;
}

// steps timer and duty values
static inline void nes_apu_sqr_step_tmr(nes_apu_sqr_t *sqr) {
  if (sqr->tmr_val == 0) {
    sqr->tmr_val = sqr->tmr_period;
    sqr->duty_val = (sqr->duty_val + 1) % 8;
  } else {
    sqr->tmr_val--;
  }
}

// steps envelope values
static inline void nes_apu_sqr_step_env(nes_apu_sqr_t *sqr) {
  if (BITMGET(sqr->flags, NES_APU_FLAG_SQR_ENV_START)) {
    sqr->env_vol = 15;
    sqr->env_val = sqr->env_period;
    sqr->flags = BITMCLR(sqr->flags, NES_APU_FLAG_SQR_ENV_START);
  } else {
    if (sqr->env_val > 0) {
      sqr->env_val--;
    } else {
      sqr->env_val = sqr->env_period;
    
      if (sqr->env_vol > 0) {
        sqr->env_vol--;
      } else if (BITMGET(sqr->flags, NES_APU_FLAG_SQR_ENV_LOOP)) {
        sqr->env_vol = 15;
      }
    }
  }
}

// applies sweep
static inline void nes_apu_sqr_sweep(nes_apu_sqr_t *sqr) {
  uint16_t delta = sqr->tmr_period >> sqr->sweep_shift;

  if (BITMGET(sqr->flags, NES_APU_FLAG_SQR_SWEEP_NEGATE)) {
    sqr->tmr_period -= delta;

    if (sqr->chan == 1)
      sqr->tmr_period--;
  }
  else {
    sqr->tmr_period += delta;
  }
}

// steps or reloads sweep value as required
static inline void nes_apu_sqr_step_sweep(nes_apu_sqr_t *sqr) {
  if (BITMGET(sqr->flags, NES_APU_FLAG_SQR_SWEEP_RELOAD)) {
    if (BITMGET(sqr->flags, NES_APU_FLAG_SQR_SWEEP_ENABLED) && !sqr->sweep_val)
      nes_apu_sqr_sweep(sqr);
    sqr->sweep_val = sqr->sweep_period;
    sqr->flags = BITMCLR(sqr->flags, NES_APU_FLAG_SQR_SWEEP_RELOAD);
  } else if (sqr->sweep_val > 0) {
    sqr->sweep_val--;
  } else {
    if (BITMGET(sqr->flags, NES_APU_FLAG_SQR_SWEEP_ENABLED))
      nes_apu_sqr_sweep(sqr);

    sqr->sweep_val = sqr->sweep_period;
  }
}

// steps signal length if needed
static inline void nes_apu_sqr_step_len(nes_apu_sqr_t *sqr) {
  if (BITMGET(sqr->flags, NES_APU_FLAG_SQR_LEN_ENABLED) && (sqr->length > 0))
    sqr->length--;
}

// returns square channel output for current APU tick
static inline uint8_t nes_apu_sqr_get_output(nes_apu_sqr_t *sqr) {
  if (!BITMGET(sqr->flags, NES_APU_FLAG_SQR_ENABLED))
    return 0;

  if (sqr->length == 0)
    return 0;

  if (duty_tbl[sqr->duty_mode][sqr->duty_val] == 0)
    return 0;

  if ((sqr->tmr_period < 8) || (sqr->tmr_period > 0x7FF))
    return 0;

  if (BITMGET(sqr->flags, NES_APU_FLAG_SQR_ENV_ENABLED)) {
    return sqr->env_vol;
  } else {
    return sqr->const_vol;
  }
}

// triangle
// consists of timer, length counter, linear counter, sequencer
// timer ticks down from T to 0 each CPU clock (!), stepping the sequencer
// each time it rolls over
// sequencer cycles through 32 values from tri_tbl and sends the current
// value on every clock straight to output (mixer)

// control register write
static inline void nes_apu_tri_write_ctrl(nes_apu_tri_t *tri, uint8_t val) {
  tri->counter_period = val & 0x7F;
  
  if (val == 0x00)
    tri->counter_val = 0;
  
  tri->flags = BITMCHG(tri->flags, NES_APU_FLAG_TRI_LEN_ENABLED,
    !((val >> 7) & 0x01));
}

// timer lo write
static inline void nes_apu_tri_write_tmr_low(nes_apu_tri_t *tri, uint8_t val) {
  tri->tmr_period = (tri->tmr_period & 0xFF00) | val;
}

// timer hi write
static inline void nes_apu_tri_write_tmr_high(nes_apu_tri_t *tri, uint8_t val) {
  tri->length = len_tbl[val >> 3];
  tri->tmr_period = (tri->tmr_period & 0x00FF) | ((uint16_t)(val & 0x07) << 8);
  tri->tmr_val = tri->tmr_period;
  tri->flags = BITMSET(tri->flags, NES_APU_FLAG_TRI_COUNTER_RELOAD);
}

// steps timer and duty values
static inline void nes_apu_tri_step_tmr(nes_apu_tri_t *tri) {
  if (tri->tmr_val == 0) {
    tri->tmr_val = tri->tmr_period;
    if ((tri->length > 0) && (tri->counter_val > 0)) {
      tri->duty_val = (tri->duty_val + 1) % 32;
      if (tri->tmr_val > 1) tri->duty_out = tri->duty_val;
    }
  } else {
    tri->tmr_val--;
  }
}

// steps the signal length if needed
static inline void nes_apu_tri_step_len(nes_apu_tri_t *tri) {
  if (BITMGET(tri->flags, NES_APU_FLAG_TRI_LEN_ENABLED) && (tri->length > 0))
    tri->length--;
}

// steps or reloads the counter as required
static inline void nes_apu_tri_step_cnt(nes_apu_tri_t *tri) {
  if (BITMGET(tri->flags, NES_APU_FLAG_TRI_COUNTER_RELOAD)) {
    tri->counter_val = tri->counter_period;
  } else if (tri->counter_val > 0) {
    tri->counter_val--;
  }
  
  if (BITMGET(tri->flags, NES_APU_FLAG_TRI_LEN_ENABLED))
    tri->flags = BITMCLR(tri->flags, NES_APU_FLAG_TRI_COUNTER_RELOAD);
}

// returns triangle channel output for current APU tick
static inline uint8_t nes_apu_tri_get_output(nes_apu_tri_t *tri) {
  if (!BITMGET(tri->flags, NES_APU_FLAG_TRI_ENABLED))
    return 0;

  if (tri->length == 0)
    return 0;

  if (tri->counter_val == 0)
    return 0;

  return tri_tbl[tri->duty_out];
}

// noise
// consists of envelope generator, timer, linear feedback shift register,
// length counter; the LFSR is 15 bits wide and is used as a pseudo random bit
// generator, loaded with 1 on start up
// each clock:
// - feedback = lfsr.0 XOR lfsr.6 if mode flag is set, else lfsr.0 XOR lfsr.1
// - lfsr >>= 1
// - lfsr.14 = feedback
// mixer receives envelope value, unless lfsr.0 == 1 or length == 0

// control register write
static inline void nes_apu_noi_write_ctrl(nes_apu_noi_t *noi, uint8_t val) {
  noi->env_period = val & 0x0F;
  noi->const_vol = val & 0x0F;
  noi->env_vol = val & 0x1F;
  noi->flags = BITMCHG(noi->flags, NES_APU_FLAG_NOI_LEN_ENABLED,
    !((val >> 5) & 0x01));
  noi->flags = BITMCHG(noi->flags, NES_APU_FLAG_NOI_ENV_LOOP,
    ((val >> 5) & 0x01));
  noi->flags = BITMCHG(noi->flags, NES_APU_FLAG_NOI_ENV_ENABLED,
    !((val >> 4) & 0x01));
  noi->flags = BITMSET(noi->flags, NES_APU_FLAG_NOI_ENV_START);
}

// timer period write
static inline void nes_apu_noi_write_prd(nes_apu_noi_t *noi, uint8_t val) {
  noi->flags = BITMCHG(noi->flags, NES_APU_FLAG_NOI_MODE, BITMGET(val, 0x80));
  noi->tmr_period = noi_tbl[val & 0x0F];
  noi->tmr_val = noi->tmr_period;
}

// signal length write
static inline void nes_apu_noi_write_len(nes_apu_noi_t *noi, uint8_t val) {
  noi->length = len_tbl[val >> 3];
  noi->flags = BITMSET(noi->flags, NES_APU_FLAG_NOI_ENV_START);
}

// steps the timer value
static inline void nes_apu_noi_step_tmr(nes_apu_noi_t *noi) {
  noi->tmr_val--;
    
  if (noi->tmr_val == 0) {
    uint16_t tmp;
    
    if (BITMGET(noi->flags, NES_APU_FLAG_NOI_MODE))
      tmp = (noi->shift & 0x40) >> 6;
    else
      tmp = (noi->shift & 0x02) >> 1;
    
    uint16_t feedback;
    
    feedback = (noi->shift & 0x01) ^ tmp;
    noi->shift = (noi->shift >> 1) | (feedback << 14);
    
    noi->tmr_val = noi->tmr_period;
  }
}

// steps envelope values
static inline void nes_apu_noi_step_env(nes_apu_noi_t *noi) {
  if (BITMGET(noi->flags, NES_APU_FLAG_NOI_ENV_START)) {
    noi->env_vol = 15;
    noi->env_val = noi->env_period;
    noi->flags = BITMCLR(noi->flags, NES_APU_FLAG_NOI_ENV_START);
  } else {
    if (noi->env_val > 0) {
      noi->env_val--;
    } else {
      noi->env_val = noi->env_period;
    
      if (noi->env_vol > 0) {
        noi->env_vol--;
      } else if (BITMGET(noi->flags, NES_APU_FLAG_NOI_ENV_LOOP)) {
        noi->env_vol = 15;
      }
    }
  }
}

// steps signal length value
static inline void nes_apu_noi_step_len(nes_apu_noi_t *noi) {
  if (BITMGET(noi->flags, NES_APU_FLAG_NOI_LEN_ENABLED) && (noi->length > 0))
    noi->length--;
}

// returns noise channel output for the current APU tick
static inline float nes_apu_noi_get_output(nes_apu_noi_t *noi) {
  if (!BITMGET(noi->flags, NES_APU_FLAG_NOI_ENABLED))
    return 0.0;

  if (noi->length == 0)
    return 0.0;

  if (noi->shift & 1)
    return 0.0;

  if (BITMGET(noi->flags, NES_APU_FLAG_NOI_ENV_ENABLED))
    return noi->env_vol;
  else
    return noi->const_vol;
}

// DMC
// outputs 1-bit delta-encoded samples
// consists of memory reader, sample buffer, shift register and output (0-127)
// reader reads next sample byte into sample buffer
// sample buffer is copied into shift register
// every timer clock output gets set based on bit 0 of shift register:
// - if 1, output += 2, else output -= 2, UNLESS this would overflow output
// - shift register >>= 1
// - if shift register is empty (8 shifts), output silence until next sample

// control register write
static inline void nes_apu_dmc_write_ctrl(nes_apu_dmc_t *dmc, uint8_t val) {
  dmc->flags = BITMCHG(dmc->flags, NES_APU_FLAG_DMC_IRQ, val & 0x80);
  dmc->flags = BITMCHG(dmc->flags, NES_APU_FLAG_DMC_LOOP, val & 0x40);
  dmc->tick_period = dmc_tbl[val & 0x0F];
}

// signal value write
static inline void nes_apu_dmc_write_value(nes_apu_dmc_t *dmc, uint8_t val) {
  dmc->value = val & 0x7F;
}

// DMC sample address write
static inline void nes_apu_dmc_write_addr(nes_apu_dmc_t *dmc, uint8_t val) {
  dmc->sample_addr = 0xC000 | ((uint16_t)val << 6);
}

// DMC sample length write
static inline void nes_apu_dmc_write_length(nes_apu_dmc_t *dmc, uint8_t val) {
  dmc->sample_length = ((uint16_t)val << 4) | 1;
}

// resets sample playback to the start of the sample
static inline void nes_apu_dmc_restart(nes_apu_dmc_t *dmc) {
  dmc->cur_addr = dmc->sample_addr;
  dmc->cur_length = dmc->sample_length;
}

// reads next byte of the sample, which takes 4 CPU clocks
// will reset playback after the end of the sample is reached,
// if the loop flag is set
// this reads the sample straight into the shift register
static inline void nes_apu_dmc_step_reader(nes_t *nes) {
  nes_apu_dmc_t *dmc = &nes->apu.dmc;
  
  if ((dmc->cur_length > 0) && (dmc->bit == 0)) {
    nes->cpu.stall += 4;
    dmc->shift = nes_mem_readb(nes, dmc->cur_addr);
    dmc->bit = 8;
    dmc->cur_addr++;
    
    if (dmc->cur_addr == 0)
      dmc->cur_addr = 0x8000;
    
    dmc->cur_length--;
    
    if ((dmc->cur_length == 0) && BITMGET(dmc->flags, NES_APU_FLAG_DMC_LOOP))
      nes_apu_dmc_restart(dmc);
  }
}

// steps the shift register as described above, modifying output
static inline void nes_apu_dmc_step_shifter(nes_apu_dmc_t *dmc) {
  if (dmc->bit == 0)
    return;
  
  if (dmc->shift & 1) {
    if (dmc->value <= 125)
      dmc->value += 2;
  } else {
    if (dmc->value >= 2)
      dmc->value -= 2;
  }
  
  dmc->shift >>= 1;
  dmc->bit--;
}

// returns output value for the current APU tick
static inline uint8_t nes_apu_dmc_get_output(nes_apu_dmc_t *dmc) {
  return dmc->value;
}

// steps the timer value (and the whole channel)
static inline void nes_apu_dmc_step_tmr(nes_t *nes) {
  if (!BITMGET(nes->apu.dmc.flags, NES_APU_FLAG_DMC_ENABLED))
    return;
  
  nes_apu_dmc_step_reader(nes);
  
  if (nes->apu.dmc.tick_value == 0) {
    nes->apu.dmc.tick_value = nes->apu.dmc.tick_period;
    nes_apu_dmc_step_shifter(&nes->apu.dmc);
  } else {
    nes->apu.dmc.tick_value--;
  }
}

// register management

// square registers write, addr is register index
static inline void nes_apu_sqr_write(nes_apu_sqr_t *sqr, uint16_t addr,
  uint8_t val) {
  switch (addr) {
    case 0: nes_apu_sqr_write_ctrl(sqr, val); break;
    case 1: nes_apu_sqr_write_sweep(sqr, val); break;
    case 2: nes_apu_sqr_write_tmr_low(sqr, val); break;
    case 3: nes_apu_sqr_write_tmr_high(sqr, val); break;
  }
}

// triangle registers write, addr is register index
static inline void nes_apu_tri_write(nes_apu_tri_t *tri, uint16_t addr,
                                     uint8_t val) {
  switch (addr) {
    case 0: nes_apu_tri_write_ctrl(tri, val); break;
    case 1:
    case 2: nes_apu_tri_write_tmr_low(tri, val); break;
    case 3: nes_apu_tri_write_tmr_high(tri, val); break;
  }
}

// noise registers write, addr is register index
static inline void nes_apu_noi_write(nes_apu_noi_t *noi, uint16_t addr,
                                     uint8_t val) {
  switch (addr) {
    case 0: nes_apu_noi_write_ctrl(noi, val); break;
    case 1: break;
    case 2: nes_apu_noi_write_prd(noi, val); break;
    case 3: nes_apu_noi_write_len(noi, val); break;
  }
}

// DMC registers write, addr is register index
static inline void nes_apu_dmc_write(nes_apu_dmc_t *dmc, uint16_t addr,
                                     uint8_t val) {
  switch (addr) {
    case 0: nes_apu_dmc_write_ctrl(dmc, val); break;
    case 1: nes_apu_dmc_write_value(dmc, val); break;
    case 2: nes_apu_dmc_write_addr(dmc, val); break;
    case 3: nes_apu_dmc_write_length(dmc, val); break;
  }
}

// tick functions

// timer tick
static inline void nes_apu_step_tmr(nes_t *nes) {
  if (nes->apu.cycle % 2 == 0) {
    nes_apu_sqr_step_tmr(&nes->apu.sq1);
    nes_apu_sqr_step_tmr(&nes->apu.sq2);
    nes_apu_noi_step_tmr(&nes->apu.noi);
    nes_apu_dmc_step_tmr(nes);
  }

  nes_apu_tri_step_tmr(&nes->apu.tri);
}

// envelope tick
static inline void nes_apu_step_env(nes_t *nes) {
  nes_apu_sqr_step_env(&nes->apu.sq1);
  nes_apu_sqr_step_env(&nes->apu.sq2);
  nes_apu_tri_step_cnt(&nes->apu.tri);
  nes_apu_noi_step_env(&nes->apu.noi);
}

// sweep tick
static inline void nes_apu_step_sweep(nes_t *nes) {
  nes_apu_sqr_step_sweep(&nes->apu.sq1);
  nes_apu_sqr_step_sweep(&nes->apu.sq2);
}

// length tick
static inline void nes_apu_step_len(nes_t *nes) {
  nes_apu_sqr_step_len(&nes->apu.sq1);
  nes_apu_sqr_step_len(&nes->apu.sq2);
  nes_apu_tri_step_len(&nes->apu.tri);
  nes_apu_noi_step_len(&nes->apu.noi);
}

// fires IRQ if required
static inline void nes_apu_fire_irq(nes_t *nes) {
  if (nes->apu.frame_irq)
    nes_cpu_irq(nes);
}

// APU frame
static inline void nes_apu_step_frame_counter(nes_t *nes) {
  nes->apu.frame_val++;
  
  if (nes->apu.frame_period == 4) {
    switch (nes->apu.frame_val) {
      case 3:
        nes_apu_fire_irq(nes);
      case 1:
        nes_apu_step_len(nes);
        nes_apu_step_sweep(nes);
      case 0:
      case 2: 
        break;
    }
  } else if (nes->apu.frame_period == 5) {
    switch (nes->apu.frame_val) {
      case 1:
      case 4:
        nes_apu_step_len(nes);
        nes_apu_step_sweep(nes);
      case 0:
      case 2: 
      case 3:
        break;
    }
  }
  
  nes_apu_step_env(nes);
  
  if (nes->apu.frame_val >= nes->apu.frame_period)
    nes->apu.frame_val = 0;
}

// returns output value for the current APU tick
static inline float nes_apu_get_output(nes_t *nes) {
  uint8_t sq1 = nes_apu_sqr_get_output(&nes->apu.sq1);
  uint8_t sq2 = nes_apu_sqr_get_output(&nes->apu.sq2);
  uint8_t tri = nes_apu_tri_get_output(&nes->apu.tri);
  uint8_t noi = nes_apu_noi_get_output(&nes->apu.noi);
  uint8_t dmc = nes_apu_dmc_get_output(&nes->apu.dmc);

  float sqs = sqr_tbl[(sq1 + sq2) % 31];
  float tnd = tnd_tbl[(3 * tri + 2 * noi + dmc) % 203];
  
  float res = 128.0 * (sqs + tnd);
  res = (res < 0.0) ? 0.0 : (res > 255.0) ? 255.0 : res;
  
  return res;
}

// appends current output to sample buffer unless it is full
static inline void nes_apu_send_sample(nes_t *nes) {
  if (nes->apu.buf == NULL)
    return;

  if (nes->apu.buf_size == nes->apu.max_buf_size)
    return;

  nes->apu.buf[nes->apu.buf_size] = nes_apu_get_output(nes);
  nes->apu.buf_size++;
}

void nes_apu_tick(nes_t *nes) {
  uint64_t cycle1 = nes->apu.cycle;
  nes->apu.cycle++;

  uint64_t cycle2 = nes->apu.cycle;
  nes_apu_step_tmr(nes);

  int f1 = (int)((double)cycle1 / nes_apu_frame_counter_rate);
  int f2 = (int)((double)cycle2 / nes_apu_frame_counter_rate);

  if (f1 != f2)
    nes_apu_step_frame_counter(nes);

  int s1 = (int)((double)cycle1 / nes_apu_sample_rate);
  int s2 = (int)((double)cycle2 / nes_apu_sample_rate);

  if (s1 != s2)
    nes_apu_send_sample(nes);
}

// APU control register write
static inline void nes_apu_write_ctrl(nes_t *nes, uint8_t val) {
  nes->apu.sq1.flags = BITMCHG(nes->apu.sq1.flags, NES_APU_FLAG_SQR_ENABLED,
    BITMGET(val, 0x01));
  nes->apu.sq2.flags = BITMCHG(nes->apu.sq2.flags, NES_APU_FLAG_SQR_ENABLED,
    BITMGET(val, 0x02));
  nes->apu.tri.flags = BITMCHG(nes->apu.tri.flags, NES_APU_FLAG_TRI_ENABLED,
    BITMGET(val, 0x04));
  nes->apu.noi.flags = BITMCHG(nes->apu.noi.flags, NES_APU_FLAG_NOI_ENABLED,
    BITMGET(val, 0x08));
  nes->apu.dmc.flags = BITMCHG(nes->apu.dmc.flags, NES_APU_FLAG_DMC_ENABLED,
    BITMGET(val, 0x10));

  if (!BITMGET(nes->apu.sq1.flags, NES_APU_FLAG_SQR_ENABLED))
    nes->apu.sq1.length = 0;
  if (!BITMGET(nes->apu.sq2.flags, NES_APU_FLAG_SQR_ENABLED))
    nes->apu.sq2.length = 0;
  if (!BITMGET(nes->apu.tri.flags, NES_APU_FLAG_TRI_ENABLED))
    nes->apu.tri.length = 0;
  if (!BITMGET(nes->apu.noi.flags, NES_APU_FLAG_NOI_ENABLED))
    nes->apu.noi.length = 0;
  if (!BITMGET(nes->apu.dmc.flags, NES_APU_FLAG_DMC_ENABLED)) {
    nes->apu.dmc.cur_length = 0;
  } else {
    if (nes->apu.dmc.cur_length == 0)
      nes_apu_dmc_restart(&nes->apu.dmc);
  }
}

// APU frame counter register write
static inline void nes_apu_write_frame_counter(nes_t *nes, uint8_t val) {
  nes->apu.frame_period = 4 + ((val >> 7) & 0x01);

  if (nes->apu.frame_period == 5)
    nes_apu_step_frame_counter(nes);
  
  nes->apu.frame_irq = !((val >> 6) & 1);
}

// APU registers write, addr is the register index
void nes_apu_write(nes_t *nes, uint16_t addr, uint8_t val) {
  if (addr < 0x04) {nes_apu_sqr_write(&nes->apu.sq1, addr, val); return;}
  if (addr < 0x08) {nes_apu_sqr_write(&nes->apu.sq2, addr - 0x04, val); return;}
  if (addr < 0x0C) {nes_apu_tri_write(&nes->apu.tri, addr - 0x08, val); return;}
  if (addr < 0x10) {nes_apu_noi_write(&nes->apu.noi, addr - 0x0C, val); return;}
  if (addr < 0x14) {nes_apu_dmc_write(&nes->apu.dmc, addr - 0x10, val); return;}
  if (addr == 0x15) {nes_apu_write_ctrl(nes, val); return;}
  if (addr == 0x17) {nes_apu_write_frame_counter(nes, val); return;}
}

// APU registers read, addr is the register index
uint8_t nes_apu_read(nes_t *nes, uint16_t addr) {
  if (addr == 0x15)
    return ((nes->apu.sq1.length > 0) ? 0x01 : 0x00) |
           ((nes->apu.sq2.length > 0) ? 0x02 : 0x00) |
           ((nes->apu.tri.length > 0) ? 0x04 : 0x00) |
           ((nes->apu.noi.length > 0) ? 0x08 : 0x00) |
           ((nes->apu.dmc.cur_length > 0) ? 0x10 : 0x00);

  return 0x00;
}
