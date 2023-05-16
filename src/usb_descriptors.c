#include <tusb.h>

//--------------------------------------------------------------------+
// Device Descriptor
//--------------------------------------------------------------------+

static tusb_desc_device_t const device_desc = {
  .bLength            = sizeof(tusb_desc_device_t),
  .bDescriptorType    = TUSB_DESC_DEVICE,
  .bcdUSB             = 0x0110,
  .bDeviceClass       = 0x00,
  .bDeviceSubClass    = 0x00,
  .bDeviceProtocol    = 0x00,
  .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
  .idVendor           = 0x6666,
  .idProduct          = 0x6868,
  .bcdDevice          = 0x0100,
  .iManufacturer      = 0x01,
  .iProduct           = 0x02,
  .iSerialNumber      = 0x00,
  .bNumConfigurations = 0x01
};

uint8_t const *tud_descriptor_device_cb() {
  return (uint8_t const *) &device_desc;
}

//--------------------------------------------------------------------+
// Configuration Descriptors
//--------------------------------------------------------------------+

#define EPNUM_MIDI_OUT 0x01
#define EPNUM_MIDI_IN  0x01

enum {
  ITF_NUM_MIDI_CONTROL = 0,
  ITF_NUM_MIDI_STREAMING,
  ITF_NUM_TOTAL
};

#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_MIDI_DESC_LEN)

uint8_t const config_desc[] = {
  TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 100),
  TUD_MIDI_DESCRIPTOR(ITF_NUM_MIDI_CONTROL, 0, EPNUM_MIDI_OUT, (0x80 | EPNUM_MIDI_IN), 64)
};

uint8_t const *tud_descriptor_configuration_cb(uint8_t index) {
  return config_desc;
}

//--------------------------------------------------------------------+
// String Descriptors
//--------------------------------------------------------------------+

static const uint16_t const *string_desc[] = {
  u"\u0409",         // 0: English
  u"midi-rate-conv", // 1: Manufacturer
  u"midi-rate-conv", // 2: Product
};

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
  static uint16_t buf[32];
  const uint16_t *desc;
  int i, len;

  if (index >= count_of(string_desc)) {
    return NULL;
  }

  desc = string_desc[index];
  for (i = 0; desc[i] && i < count_of(buf) - 1; i++) {
    buf[i + 1] = desc[i];
  }
  len = i;

  buf[0] = (TUSB_DESC_STRING << 8) | ((len + 1) * 2);

  return buf;
}