#ifndef RING_BUFFER_H
#define RING_BUFFER_H

typedef struct {
  size_t pos;
  size_t length;
  size_t capacity;
  uint8_t *buffer;
} ring_buffer_t;

void ring_buffer_init(ring_buffer_t *ctx, uint8_t *buffer, size_t len);
void ring_buffer_write(ring_buffer_t *ctx, uint8_t *data, size_t len);
size_t ring_buffer_peek(ring_buffer_t *ctx, uint8_t *data, size_t len);
size_t ring_buffer_read(ring_buffer_t *ctx, uint8_t *data, size_t len);
void ring_buffer_clear(ring_buffer_t *ctx);
size_t ring_buffer_get_length(ring_buffer_t *ctx);
bool ring_buffer_is_empty(ring_buffer_t *ctx);
bool ring_buffer_is_full(ring_buffer_t *ctx);

#endif /* RING_BUFFER_H */