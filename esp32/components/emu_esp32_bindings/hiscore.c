//Hiscore get/put routines
/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
 * this notice you can do whatever you want with this stuff. If we meet some day, 
 * and you think this stuff is worth it, you can buy me a beer in return. 
 * ----------------------------------------------------------------------------
 */

#include <string.h>
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"

void hiscore_get(const char *file, uint8_t *data) {
	nvs_handle_t h;
	for (int i=0; i<64; i+=16) {
		memset(&data[i], 0, 16);
		data[i+11]=i/16+1;
		data[i+12]='A';
		data[i+13]='S';
		data[i+14]='S';
	}
	nvs_open("pfhi", NVS_READONLY, &h);
	size_t l=16*4;
	nvs_get_blob(h, file, data, &l);
	nvs_close(h);
}


void hiscore_put(const char *file, const uint8_t *data) {
	nvs_handle_t h;
	nvs_open("pfhi", NVS_READWRITE, &h);
	nvs_set_blob(h, file, data, 16*4);
	nvs_close(h);
}
