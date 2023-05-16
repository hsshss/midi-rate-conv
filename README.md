# Overview

Converts RS-MIDI signals (38400bps) to MIDI signals (31250bps).

# Schematics

https://oshwlab.com/hsshss/midi-rate-conv

# Building

```bash
git clone https://github.com/hsshss/midi-rate-conv.git
cd midi-rate-conv
docker build -t midi-rate-conv-builder .
docker run --rm -v ${PWD}:/app -t midi-rate-conv-builder sh -c "mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && cmake --build ."
```

# Flashing

1. Disconnect all connectors
2. While holding down the BOOTSEL button and connect to PC with USB cable
3. A USB Mass Storage Device named `RPI-RP2` will be mounted
4. Drag and drop `midi-rate-conv.uf2` file to the `RPI-RP2`
