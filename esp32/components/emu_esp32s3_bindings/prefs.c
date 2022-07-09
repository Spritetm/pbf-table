//Pref bindings. We read/write them to NVS.
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
#include "prefs.h"

void prefs_read(pref_type_t *p) {
	nvs_handle_t h;
	nvs_open("pref", NVS_READONLY, &h);
	size_t l=sizeof(pref_type_t);
	nvs_get_blob(h, "pref", p, &l);
	nvs_close(h);
}


void prefs_write(pref_type_t *p) {
	nvs_handle_t h;
	nvs_open("pref", NVS_READWRITE, &h);
	nvs_set_blob(h, "pref", p, sizeof(pref_type_t));
	nvs_close(h);
}
