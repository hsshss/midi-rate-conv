#ifndef SC55_EMU_H
#define SC55_EMU_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

typedef void (*sc55_emu_write_cb_t)(const uint8_t *buffer, size_t len);

typedef struct {
  sc55_emu_write_cb_t write_cb;
  bool passthrough;
  int buffer_pos;
  uint8_t cc0[16];
  uint8_t drum[16];
  uint16_t rpn[16];
  uint16_t nrpn[16];
  uint8_t buffer[16];
} sc55_emu_t;

void sc55_emu_init(sc55_emu_t *ctx, sc55_emu_write_cb_t write_cb);
void sc55_emu_write(sc55_emu_t *ctx, const uint8_t *buffer, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* SC55_EMU_H */