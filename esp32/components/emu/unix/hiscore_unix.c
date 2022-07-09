//Hiscore driver. Mostly stubbed.
/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
 * this notice you can do whatever you want with this stuff. If we meet some day, 
 * and you think this stuff is worth it, you can buy me a beer in return. 
 * ----------------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hiscore.h"

void hiscore_get(const char *file, uint8_t *data) {
	printf("Stub: hiscore_get %s\n", file);
	for (int i=0; i<64; i+=16) {
		memset(&data[i], 0, 16);
		data[i+11]=i/16+1;
		data[i+12]='A';
		data[i+13]='S';
		data[i+14]='S';
	}
}


void hiscore_put(const char *file, const uint8_t *data) {
	printf("Stub: hiscore_put %s\n", file);
}
