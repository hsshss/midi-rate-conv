#include <hardware/gpio.h>
#include <pico/multicore.h>
#include <tusb.h>
#include "config.h"

void uart_midi_init();
void uart_midi_task();
void usb_midi_task();

bool sc55_emu_enabled = false;

void core1_entry() {
  while (true) {
    uart_midi_task();
  }
}

static void init() {
  gpio_init(SC55_EMU_PIN);
  gpio_set_dir(SC55_EMU_PIN, GPIO_IN);
  gpio_pull_up(SC55_EMU_PIN);
}

void main() {
  init();
  uart_midi_init();
  tusb_init();

  multicore_launch_core1(core1_entry);

  while (true) {
    sc55_emu_enabled = !gpio_get(SC55_EMU_PIN);
    usb_midi_task();
    tud_task();
  }
}