#include <hardware/gpio.h>
#include <hardware/timer.h>
#include <hardware/uart.h>
#include "config.h"
#include "ring_buffer.h"

static uint8_t buffer_data[MIDI_IN_BAUD_RATE / 100];
static ring_buffer_t buffer;
static uint64_t data_ind = 0;
static uint64_t error_ind = 0;

static void init() {
  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);
  gpio_put(LED_PIN, 1);

  ring_buffer_init(&buffer, buffer_data, sizeof(buffer_data));

  gpio_set_function(MIDI_IN_PIN, GPIO_FUNC_UART);
  gpio_set_function(MIDI_OUT_PIN, GPIO_FUNC_UART);
  gpio_set_drive_strength(MIDI_OUT_PIN, GPIO_DRIVE_STRENGTH_12MA);

  uart_init(MIDI_IN_UART, MIDI_IN_BAUD_RATE);
  uart_set_hw_flow(MIDI_IN_UART, false, false);
  uart_set_format(MIDI_IN_UART, 8, 1, UART_PARITY_NONE);
  uart_set_fifo_enabled(MIDI_IN_UART, true);

  uart_init(MIDI_OUT_UART, MIDI_OUT_BAUD_RATE);
  uart_set_hw_flow(MIDI_OUT_UART, false, false);
  uart_set_format(MIDI_OUT_UART, 8, 1, UART_PARITY_NONE);
  uart_set_fifo_enabled(MIDI_OUT_UART, true);
}

void receive_task() {
  uint8_t c;
  bool received = false;

  while (uart_is_readable(MIDI_IN_UART)) {
    c = uart_getc(MIDI_IN_UART);
    ring_buffer_write(&buffer, &c, 1);
    if (ring_buffer_is_full(&buffer)) {
      error_ind = time_us_64() + ERROR_IND_TIME;
    }
    received = true;
  }

  if (received) {
    data_ind = time_us_64() + DATA_IND_TIME;
  }
}

void send_task() {
  uint8_t c;

  while (uart_is_writable(MIDI_OUT_UART)) {
    size_t len = ring_buffer_read(&buffer, &c, 1);
    if (len == 0) {
      break;
    }

    uart_putc_raw(MIDI_OUT_UART, c);
  }
}

void blink_task(void) {
  uint64_t current = time_us_64();
  gpio_put(LED_PIN, !(data_ind > current || error_ind > current));
}

void main() {
  init();

  while (true) {
    receive_task();
    send_task();
    blink_task();
  }
}