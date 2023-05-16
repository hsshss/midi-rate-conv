#include <string.h>
#include <pico/stdlib.h>
#include "ring_buffer.h"

void ring_buffer_init(ring_buffer_t *ctx, uint8_t *buffer, size_t size) {
  ctx->pos = 0;
  ctx->length = 0;
  ctx->capacity = size;
  ctx->buffer = buffer;
}

void ring_buffer_write(ring_buffer_t *ctx, uint8_t *data, size_t len) {
  if (len == 0) {
    return;
  }

  if (len > ctx->capacity) {
    data += len - ctx->capacity;
    len = ctx->capacity;
  }

  if (ctx->pos + len > ctx->capacity) {
    size_t size = ctx->capacity - ctx->pos;
    memcpy(ctx->buffer + ctx->pos, data, size);
    memcpy(ctx->buffer, data + size, len - size);
    ctx->pos = (ctx->pos + len) % ctx->capacity;
    ctx->length += len;
    ctx->length = ctx->length <= ctx->capacity ? ctx->length : ctx->capacity;
  } else {
    memcpy(ctx->buffer + ctx->pos, data, len);
    ctx->pos = (ctx->pos + len) % ctx->capacity;
    ctx->length += len;
    ctx->length = ctx->length <= ctx->capacity ? ctx->length : ctx->capacity;
  }
}

size_t ring_buffer_peek(ring_buffer_t *ctx, uint8_t *data, size_t len) {
  if (len == 0) {
    return 0;
  }

  if (len > ctx->length) {
    len = ctx->length;
  }

  size_t start_pos = (ctx->pos + ctx->capacity - ctx->length) % ctx->capacity;

  if (start_pos + len > ctx->capacity) {
    size_t size = ctx->capacity - start_pos;
    memcpy(data, ctx->buffer + start_pos, size);
    memcpy(data + size, ctx->buffer, len - size);
  } else {
    memcpy(data, ctx->buffer + start_pos, len);
  }

  return len;
}

size_t ring_buffer_read(ring_buffer_t *ctx, uint8_t *data, size_t len) {
  if (data != NULL) {
    size_t res = ring_buffer_peek(ctx, data, len);
    ctx->length -= res;
    return res;
  } else {
    size_t res = MIN(len, ctx->length);
    ctx->length -= res;
    return res;
  }
}

void ring_buffer_clear(ring_buffer_t *ctx) {
  ctx->pos = 0;
  ctx->length = 0;
  memset(ctx->buffer, 0, ctx->capacity);
}

size_t ring_buffer_get_length(ring_buffer_t *ctx) {
  return ctx->length;
}

bool ring_buffer_is_empty(ring_buffer_t *ctx) {
  return ctx->length == 0;
}

bool ring_buffer_is_full(ring_buffer_t *ctx) {
  return ctx->length == ctx->capacity;
}