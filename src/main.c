#include <pico/multicore.h>
#include <tusb.h>

void uart_midi_init();
void uart_midi_task();
void usb_midi_task();

void core1_entry() {
  while (true) {
    uart_midi_task();
  }
}

void main() {
  uart_midi_init();
  tusb_init();

  multicore_launch_core1(core1_entry);

  while (true) {
    usb_midi_task();
    tud_task();
  }
}