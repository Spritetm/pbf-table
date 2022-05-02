#!/bin/bash
esptool.py --chip esp32s3 --port "/dev/ttyUSB0" --baud $((921600/2)) \
	--before default_reset --after hard_reset write_flash  \
	0x190000 ../data/TABLE1.PRG \
	0x230000 ../data/TABLE2.PRG \
	0x2d0000 ../data/TABLE3.PRG \
	0x370000 ../data/TABLE4.PRG \
	0x410000 ../data/TABLE1.MOD \
	0x460000 ../data/TABLE2.MOD \
	0x4b0000 ../data/TABLE3.MOD \
	0x500000 ../data/TABLE4.MOD \
	0x550000 ../data/INTRO.MOD \
	0x5a0000 ../data/table-screens.bin
