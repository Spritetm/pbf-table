# Pinball Fantasies data files

This folder should contain the data files to run Pinball Fantasies, plus 'table-screens.bin' which
contains screenshots of the various tables in VGA memory dump format.

# How to get the data files

Easiest legal way:
- Buy the Pinball Gold Pack on GOG: https://www.gog.com/en/game/pinball_gold_pack
- This should make Pinball Fantasies Deluxe show up in your library. Download and install it.
- From the installed Pinball Fantasies Deluxe directory, find data/game.gog.
- Copy data/game.gog into this directory.
- Make sure you have Dosbox installed.
- Start dosbox in the current directory
- The dosbox main screen should pop up with '12 files copied' and the command prompt at the bottom
  of the screen. This means the files were succesfully extracted.
- Enter 'exit' to close dosbox.
- Remove game.gog.

# How to rebuild 'table-screens.bin'

The file is included in the Git repo as I imagine some screenshots easily fall under 'fair use',
but in case you want to rebuild it, here's how:

- Compile Unix build of the emulator (it's located in esp32/components/main/emu, refer to
  the README.md there to build it)
- Load up table 1
- Press 't' to dump a screenshot of the table as vram.bin and vram.pal
- Rename those files to vram1.bin and vram1.pal
- Repeat for tables 2, 3, 4
- cat vram1.bin vram1.pal vram2.bin vram2.pal vram3.bin vram3.pal vram4.bin vram4.pal > table-screens.bin

