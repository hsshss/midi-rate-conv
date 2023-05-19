#include <string.h>
#include <pico/platform.h>
#include "sc55_emu.h"
#include "sc55_map.h"

#define STATUS_CC           0xb0
#define STATUS_PC           0xc0
#define STATUS_SYSEX_START  0xf0
#define STATUS_SYSEX_END    0xf7
#define CC_BANK_MSB         0
#define CC_DATA_ENTRY_MSB   6
#define CC_BANK_LSB         32
#define CC_NRPN_LSB         98
#define CC_NRPN_MSB         99
#define CC_RPN_LSB          100
#define CC_RPN_MSB          101
#define CC_RESET_ALL_CTRL   121
#define PROG_DRUM_JAZZ      PROG_NO(33)
#define PROG_DRUM_INVALID   PROG_NO(127)
#define INVALID_BANK        113
#define NRPN_TVF_CO_FREQ    0x0120
#define MAP_SC55            1

#define MIDI_CH(x)          ((x) - 1)

static void clear_all_state(sc55_emu_t *ctx);
static void process_cc(sc55_emu_t *ctx, const uint8_t *data);
static void process_pc(sc55_emu_t *ctx, const uint8_t *data);
static void process_sysex(sc55_emu_t *ctx, const uint8_t *data, size_t len);
static uint8_t find_bank(uint8_t prog, uint8_t bank);
static uint8_t find_drum(uint8_t prog);

static const uint8_t gm_system_on[] = {0xf0, 0x7e, 0x7f, 0x09, 0x01, 0xf7};
static const uint8_t gs_reset[] = {0xf0, 0x41, 0x10, 0x42, 0x12, 0x40, 0x00, 0x7f, 0x00, 0x41, 0xf7};

void sc55_emu_init(sc55_emu_t *ctx, sc55_emu_write_cb_t write_cb) {
  memset(ctx, 0, sizeof(sc55_emu_t));
  ctx->write_cb = write_cb;
  ctx->passthrough = true;
  clear_all_state(ctx);
}

void sc55_emu_write(sc55_emu_t *ctx, const uint8_t *data, size_t len) {
  uint8_t c, status;

  for (int i = 0; i < len; i++) {
    c = data[i];

    if (c & 0x80 && c != STATUS_SYSEX_END) {
      ctx->passthrough = true;
      ctx->buffer_pos = 0;

      switch (c & 0xf0) {
        case STATUS_CC:
        case STATUS_PC:
          ctx->passthrough = false;
          break;
      }
    }

    if (ctx->passthrough) {
      ctx->write_cb(&c, 1);
    }

    if (ctx->buffer_pos < sizeof(ctx->buffer)) {
      ctx->buffer[ctx->buffer_pos] = c;
      ctx->buffer_pos++;
    }

    status = ctx->buffer[0] & 0xf0;

    if (c == STATUS_SYSEX_END && ctx->buffer[0] == STATUS_SYSEX_START) {
      process_sysex(ctx, ctx->buffer, ctx->buffer_pos);
    } else if (status == STATUS_CC && ctx->buffer_pos == 3) {
      process_cc(ctx, ctx->buffer);
    } else if (status == STATUS_PC && ctx->buffer_pos == 2) {
      process_pc(ctx, ctx->buffer);
    }
  }
}

inline static uint8_t block2part(uint8_t block) {
  if (block == 0) {
    return MIDI_CH(10);
  } else if (block <= 9) {
    return block - 1;
  } else {
    return block;
  }
}

static void clear_all_state(sc55_emu_t *ctx) {
  memset(ctx->cc0, 0, sizeof(ctx->cc0));
  memset(ctx->drum, 0, sizeof(ctx->drum));
  memset(ctx->rpn, 0x7f, sizeof(ctx->rpn));
  memset(ctx->nrpn, 0x7f, sizeof(ctx->nrpn));
  ctx->drum[MIDI_CH(10)] = 1;
}

static void reset_all_controller(sc55_emu_t *ctx, uint8_t channel) {
  ctx->rpn[channel] = 0x7f7f;
  ctx->nrpn[channel] = 0x7f7f;
}

static void process_cc(sc55_emu_t *ctx, const uint8_t *data) {
  static uint8_t buffer[3];
  uint8_t channel;

  channel = data[0] & 0x0f;

  switch (data[1]) {
    case CC_BANK_MSB:
      ctx->cc0[channel] = data[2];
      break;

    case CC_BANK_LSB:
      // ignore
      break;

    case CC_NRPN_MSB:
      ctx->nrpn[channel] = ctx->nrpn[channel] & 0x00ff | data[2] << 8;
      ctx->write_cb(data, 3);
      break;

    case CC_NRPN_LSB:
      ctx->nrpn[channel] = ctx->nrpn[channel] & 0xff00 | data[2];
      ctx->write_cb(data, 3);
      break;

    case CC_RPN_MSB:
      ctx->rpn[channel] = ctx->rpn[channel] & 0x00ff | data[2] << 8;
      if (ctx->rpn[channel] == 0x7f7f) {
        ctx->nrpn[channel] = 0x7f7f;
      }
      ctx->write_cb(data, 3);
      break;

    case CC_RPN_LSB:
      ctx->rpn[channel] = ctx->rpn[channel] & 0xff00 | data[2];
      if (ctx->rpn[channel] == 0x7f7f) {
        ctx->nrpn[channel] = 0x7f7f;
      }
      ctx->write_cb(data, 3);
      break;

    case CC_DATA_ENTRY_MSB:
      if (ctx->nrpn[channel] == NRPN_TVF_CO_FREQ) {
        buffer[0] = data[0];
        buffer[1] = data[1];
        buffer[2] = MIN(data[2], 64);
        ctx->write_cb(buffer, 3);
      } else {
        ctx->write_cb(data, 3);
      }
      break;

    case CC_RESET_ALL_CTRL:
      reset_all_controller(ctx, channel);
      ctx->write_cb(data, 3);
      break;

    default:
      ctx->write_cb(data, 3);
  }
}

static void process_pc(sc55_emu_t *ctx, const uint8_t *data) {
  static uint8_t buffer[3];
  uint8_t channel, prog;

  channel = data[0] & 0x0f;
  prog = data[1];

  if (ctx->drum[channel]) {
    buffer[0] = STATUS_CC | channel;
    buffer[1] = CC_BANK_MSB;
    buffer[2] = 0;
    ctx->write_cb(buffer, 3);

    buffer[0] = STATUS_CC | channel;
    buffer[1] = CC_BANK_LSB;
    buffer[2] = MAP_SC55;
    ctx->write_cb(buffer, 3);

    buffer[0] = STATUS_PC | channel;
    buffer[1] = find_drum(prog);
    ctx->write_cb(buffer, 2);
  } else {
    buffer[0] = STATUS_CC | channel;
    buffer[1] = CC_BANK_MSB;
    buffer[2] = find_bank(prog, ctx->cc0[channel]);
    ctx->write_cb(buffer, 3);

    buffer[0] = STATUS_CC | channel;
    buffer[1] = CC_BANK_LSB;
    buffer[2] = MAP_SC55;
    ctx->write_cb(buffer, 3);

    buffer[0] = STATUS_PC | channel;
    buffer[1] = prog;
    ctx->write_cb(buffer, 2);
  }
}

static bool is_reset(const uint8_t *data, size_t len) {
  return
      len == sizeof(gm_system_on) &&
      memcmp(data, gm_system_on, sizeof(gm_system_on)) == 0 ||
      len == sizeof(gs_reset) &&
      memcmp(data, gs_reset, sizeof(gs_reset)) == 0;
}

static bool is_sysex_dt1(const uint8_t *data, size_t len) {
  return data[1] == 0x41 && data[3] == 0x42 && data[4] == 0x12;
}

static void process_sysex(sc55_emu_t *ctx, const uint8_t *data, size_t len) {
  if (is_reset(data, len)) {
    clear_all_state(ctx);
  } else if (
      is_sysex_dt1(data, len) &&
      data[5] == 0x40 && (data[6] & 0xf0) == 0x10 && data[7] == 0x15) {
    // SysEx: USE FOR RHYTHM PART
    uint8_t block = data[6] & 0x0f;
    uint8_t drum_map = data[8];
    ctx->drum[block2part(block)] = drum_map;
  }
}

static bool is_valid_tone(const uint8_t *tones, int number) {
  uint8_t tone;

  if (number == 0) {
    return true;
  }

  for (int i = 1; tones[i] <= 127; i++) {
    tone = tones[i];

    if (tone == number) {
      return true;
    }
  }

  return false;
}

static uint8_t find_similar_tone(const uint8_t *tones, int number) {
  uint8_t tone;
  uint8_t alternate = 0;

  if (number == 0) {
    return 0;
  }

  for (int i = 1; tones[i] <= 127; i++) {
    tone = tones[i];

    if (tone == number) {
      return tone;
    } else if ((tone & 0xf8) == (number & 0xf8)) {
      alternate = tone;
    }
  }

  return alternate;
}

static uint8_t find_bank(uint8_t prog, uint8_t bank) {
  if (bank == 0) {
    return 0;
  } else if (bank >= 64 || prog >= PROG_NO(121)) {
    if (is_valid_tone(inst_map[prog], bank)) {
      return bank;
    } else {
      return INVALID_BANK;
    }
  }

  return find_similar_tone(inst_map[prog], bank);
}

static uint8_t find_drum(uint8_t prog) {
  if (prog == 0 || prog == PROG_DRUM_JAZZ) {
    return 0;
  } else if (
      prog >= PROG_NO(50) && prog <= PROG_NO(56) ||
      prog >= PROG_NO(58) && prog <= PROG_NO(127)) {
    if (is_valid_tone(drums, prog)) {
      return prog;
    } else {
      return PROG_DRUM_INVALID;
    }
  }

  return find_similar_tone(drums, prog);
}