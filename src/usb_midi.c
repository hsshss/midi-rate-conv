#include <tusb.h>

size_t usb_in_buffer_peek(uint8_t *buffer, size_t len);
size_t usb_in_buffer_read(uint8_t *buffer, size_t len);
void rs_out_buffer_write(uint8_t *buffer, size_t len);

static void usb_midi_in_task() {
  static uint8_t buffer[32];
  size_t len, res;

  while (true) {
    len = usb_in_buffer_peek(buffer, sizeof(buffer));
    if (len == 0) {
      break;
    }

    res = tud_midi_stream_write(0, buffer, len);
    usb_in_buffer_read(NULL, res);

    if (len != res) {
      break;
    }
  }
}

static void usb_midi_out_task() {
  static uint8_t buffer[32];
  size_t len;

  while (true) {
    len = tud_midi_stream_read(buffer, sizeof(buffer));
    if (len == 0) {
      break;
    }

    rs_out_buffer_write(buffer, len);
  }
}

void usb_midi_task() {
  usb_midi_in_task();
  usb_midi_out_task();
}