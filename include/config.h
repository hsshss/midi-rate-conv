#ifndef CONFIG_H
#define CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#define DATA_IND_TIME       (10 * 1000) // us
#define ERROR_IND_TIME      (5 * 1000 * 1000) // us

#define RS_MIDI_BAUD_RATE   38400
#define MIDI_BAUD_RATE      31250

#define RS_MIDI_UART        uart0
#define MIDI_UART           uart1

#define RS_MIDI_OUT_PIN     0
#define RS_MIDI_IN_PIN      1
#define MIDI_OUT_PIN        4
#define IND_LED_PIN         25
#define SC55_EMU_PIN        26

#ifdef __cplusplus
}
#endif

#endif /* CONFIG_H */