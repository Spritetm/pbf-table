# Pinball Fantasies physical virtual pinball table

These are the firmware, case designs, schematics and PCB artwork
for the tiny video pinball table as described [here](https://spritesmods.com?art=pftable).

# Firmware

The firmware consists of a core emulator that emulates the main game
logic, a music player to replace the audio subsystem, and bindings
for graphics, input, and audio playback for various situations.

There are three builds for the firmware:

- Host build. For this, make sure you have the SDL2 development libraries
  installed and run ``make`` ``in the esp32/components/emu`` folder. You
  should get an ``emu`` binary using that. Use lshift, rshift, F1 and 
  enter to control the emulator.
- ESP32 build for ESP-Wrover-Kit
  This needs a wrover with >=8MiB of flash (and >=2MiB of psram). Use
  ``idf.py set-target esp32`` to select this and build and flash 
  using standard esp-idf procedures. You need buttons to control these,
  for start, plunger, left flipper and right flipper respectively, 
  connect buttons from GPIO 14, 13, 15 and 12 to ground. Audio signal
  comes out of GPIO26. Note that the game does not run at full frame
  rate on an ESP32 as the code is not optimized for that chip.
- ESP32S3 on custom PCB.
  This needs the pcbs from the ``pcb/`` and ``pcb-backbox-large/`` fabricated
  and assembled. Use ``idf.py set-target esp32s3`` and build and flash
  according to standard esp-idf procedures.

Note that the ESP32 and ESP32-S3 targets also need the Pinball 
Fantasies data files flashed; the Unix build simply needs them to
exist in the data directory. Details and helpers for obtaining these
files is in the ``data/README.md`` file. There is an example script for
flashing in ``esp32/flashdata.sh``, but it may need modifying for your
particular setup.

# Case

There is a case design (made in OpenSCAD) in the ``case`` subdirectory. 
Note that the measures are mostly what works on my 3d-printer; you 
may need to print this, see if it fits, then print again to get a 
working case.

The case parts are held together by two m3 screws fitting in two 4x5mm
brass screw thread inserts. I simply super-glued the inserts in place.

# PCB

There are three folders with Kicad PCBs: ``pcb``, ``pcb-backboard`` and 
``pcb-backboard-large``. ``pcb`` contains the main PCB which fits under
the main playfield LCD. ``pcb-backboard-large`` contains the PCB I used
for the final backboard, it interfaces with a 2.3" 320x240 LCD. ``pcb-backboard``
is a previous iteration of this; it has an 1.5" 240x240 LCD on it; it 
is included for reference only.

# Components

Most of the components are indicated on the PCBs, but some are connected
using JST-PH or JST-XH connectors. Here's a short list of what goes where:

- J2: Speaker. I used a 4 ohm, 1.5W speaker from a laptop I had around.
- J3: Connected to an 10uF MLCC SMD cap used as the plunger sensor. The 
  plunger mechanics physically hit this cap when slamming back. See
  article on how that works.
- J4: LiIon/LiPo battery. I used a generic 1000mAh, 3.7V cell I had around.
- J5: Horizontal and vertical linear haptic actuators. These actuators are
  spare parts for a Huawei Mate 30 Pro.
- J6: Flipper and start buttons. These are a 4*5.6 HD11 microswitch each.
  Note that if you use something else, you may need to change the case model
  to make them fit.




