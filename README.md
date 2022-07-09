

There are three builds for the firmware:
- Host build
- ESP32 build for ESP-Wrover-Kit
  This needs a wrover with >=8MiB of flash (and >=2MiB of psram). Use
  ``idf.py set-target esp32`` to select this and build and flash 
  using standard esp-idf procedures. You need buttons to control these,
  for start, plunger, left flipper and right flipper respectively, 
  connect buttons from GPIO 14, 13, 15 and 12 to ground. Audio signal
  comes out of GPIO26. Note that the game does not run at full frame
  rate on an ESP32 as the code is not optimized for that chip.
- ESP32S3 on custom PCB.
  This needs the pcbs from the pcb/ and pcb-backbox-large/ fabricated
  and assembled. Use ``idf.py set-target esp32s3`` and build and flash
  according to standard esp-idf procedures.

Note that the ESP32 and ESP32-S3 targets also need the Pinball 
Fantasies data files flashed. Details and helpers for obtaining these
files is in the data/README.md file. There is an example script for
flashing in esp32/flashdata.sh, but it may need modifying for your
particular setup.


