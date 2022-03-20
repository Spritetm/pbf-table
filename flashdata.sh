#!/bin/bash
esptool.py --chip esp32 --port "/dev/ttyUSB1" --baud $((921600/2)) \
	--before default_reset --after hard_reset write_flash  \
	0x100000 ../data/TABLE1.PRG \
	0x200000 ../data/TABLE2.PRG \
	0x300000 ../data/TABLE3.PRG \
	0x400000 ../data/TABLE4.PRG \
	0x500000 ../data/TABLE1.MOD \
	0x600000 ../data/TABLE2.MOD \
	0x700000 ../data/TABLE3.MOD \
	0x800000 ../data/TABLE4.MOD \
	0x900000 ../data/INTRO.MOD \
