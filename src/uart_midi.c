#include <hardware/gpio.h>
#include <hardware/timer.h>
#include <hardware/uart.h>
#include <pico/sync.h>
#include "config.h"
#include "ring_buffer.h"

static critical_section_t usb_in_buffer_lock;
static critical_section_t rs_out_buffer_lock;
static uint8_t usb_in_buffer_data[RS_MIDI_BAUD_RATE / 100];
static uint8_t rs_out_buffer_data[RS_MIDI_BAUD_RATE / 100];
static uint8_t midi_out_buffer_data[RS_MIDI_BAUD_RATE / 100];
static ring_buffer_t usb_in_buffer;
static ring_buffer_t rs_out_buffer;
static ring_buffer_t midi_out_buffer;
static uint64_t data_ind = 0;
static uint64_t error_ind = 0;

void uart_midi_init() {
  gpio_init(IND_LED_PIN);
  gpio_set_dir(IND_LED_PIN, GPIO_OUT);
  gpio_put(IND_LED_PIN, 1);

  critical_section_init(&usb_in_buffer_lock);
  critical_section_init(&rs_out_buffer_lock);

  ring_buffer_init(&usb_in_buffer, usb_in_buffer_data, sizeof(usb_in_buffer_data));
  ring_buffer_init(&rs_out_buffer, rs_out_buffer_data, sizeof(rs_out_buffer_data));
  ring_buffer_init(&midi_out_buffer, midi_out_buffer_data, sizeof(midi_out_buffer_data));

  gpio_set_function(RS_MIDI_OUT_PIN, GPIO_FUNC_UART);
  gpio_set_function(RS_MIDI_IN_PIN, GPIO_FUNC_UART);

  gpio_set_function(MIDI_OUT_PIN, GPIO_FUNC_UART);
  gpio_set_drive_strength(MIDI_OUT_PIN, GPIO_DRIVE_STRENGTH_12MA);

  uart_init(RS_MIDI_UART, RS_MIDI_BAUD_RATE);
  uart_set_hw_flow(RS_MIDI_UART, false, false);
  uart_set_format(RS_MIDI_UART, 8, 1, UART_PARITY_NONE);
  uart_set_fifo_enabled(RS_MIDI_UART, false);

  uart_init(MIDI_UART, MIDI_BAUD_RATE);
  uart_set_hw_flow(MIDI_UART, false, false);
  uart_set_format(MIDI_UART, 8, 1, UART_PARITY_NONE);
  uart_set_fifo_enabled(MIDI_UART, false);
}

static void rs_midi_rx_task() {
  uint8_t c;
  bool received = false;

  while (uart_is_readable(RS_MIDI_UART)) {
    c = uart_getc(RS_MIDI_UART);

    critical_section_enter_blocking(&usb_in_buffer_lock);
    ring_buffer_write(&usb_in_buffer, &c, 1);
    critical_section_exit(&usb_in_buffer_lock);

    ring_buffer_write(&midi_out_buffer, &c, 1);
    if (ring_buffer_is_full(&midi_out_buffer)) {
      error_ind = time_us_64() + ERROR_IND_TIME;
    }

    received = true;
  }

  if (received) {
    data_ind = time_us_64() + DATA_IND_TIME;
  }
}

static void rs_midi_tx_task() {
  uint8_t c;
  bool sent = false;

  while (uart_is_writable(RS_MIDI_UART)) {
    size_t len = ring_buffer_read(&rs_out_buffer, &c, 1);
    if (len == 0) {
      break;
    }

    uart_putc_raw(RS_MIDI_UART, c);
    sent = true;
  }

  if (sent) {
    data_ind = time_us_64() + DATA_IND_TIME;
  }
}

static void midi_out_task() {
  uint8_t c;

  while (uart_is_writable(MIDI_UART)) {
    size_t len = ring_buffer_read(&midi_out_buffer, &c, 1);
    if (len == 0) {
      break;
    }

    uart_putc_raw(MIDI_UART, c);
  }
}

static void blink_task(void) {
  uint64_t current = time_us_64();
  gpio_put(IND_LED_PIN, !(data_ind > current || error_ind > current));
}

void uart_midi_task() {
  rs_midi_rx_task();
  midi_out_task();
  rs_midi_tx_task();
  blink_task();
}

size_t usb_in_buffer_peek(uint8_t *buffer, size_t len) {
  critical_section_enter_blocking(&usb_in_buffer_lock);
  size_t ret = ring_buffer_peek(&usb_in_buffer, buffer, len);
  critical_section_exit(&usb_in_buffer_lock);
  return ret;
}

size_t usb_in_buffer_read(uint8_t *buffer, size_t len) {
  critical_section_enter_blocking(&usb_in_buffer_lock);
  size_t ret = ring_buffer_read(&usb_in_buffer, buffer, len);
  critical_section_exit(&usb_in_buffer_lock);
  return ret;
}

void rs_out_buffer_write(uint8_t *buffer, size_t len) {
  critical_section_enter_blocking(&rs_out_buffer_lock);
  ring_buffer_write(&rs_out_buffer, buffer, len);
  critical_section_exit(&rs_out_buffer_lock);
}