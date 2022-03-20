#include <stdio.h>
#include <esp_log.h>
#include <gfx.h>
#include "lcd.h"

void gfx_init() {
	lcd_init();
}

int gfx_get_key() {
	return 0;
}


void gfx_show(uint8_t *buf, uint32_t *pal, int h, int w, int scroll) {
	lcd_show(buf, pal, h, w, scroll);
}

