#!/bin/bash
esptool.py --chip esp32s3 --port "/dev/ttyUSB0" --baud $((921600/2)) \
	--before default_reset --after hard_reset write_flash  \
	0x100000 ../data/TABLE1.PRG \
	0x1a0000 ../data/TABLE2.PRG \
	0x240000 ../data/TABLE3.PRG \
	0x2e0000 ../data/TABLE4.PRG \
	0x380000 ../data/TABLE1.MOD \
	0x3d0000 ../data/TABLE2.MOD \
	0x420000 ../data/TABLE3.MOD \
	0x470000 ../data/TABLE4.MOD \
	0x4c0000 ../data/INTRO.MOD \
