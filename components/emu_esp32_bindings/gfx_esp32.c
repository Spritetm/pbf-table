#include <stdio.h>
#include <esp_log.h>
#include <gfx.h>
#include "lcd.h"
#include "driver/gpio.h"

#define BTN_LEFT 15
#define BTN_START 14
#define BTN_PLUNGER 13
#define BTN_RIGHT 12

void gfx_init() {
	lcd_init();
	gpio_config_t bconfig={
		.pin_bit_mask=(1<<BTN_LEFT)|(1<<BTN_START)|(1<<BTN_PLUNGER)|(1<<BTN_RIGHT),
		.mode=GPIO_MODE_DEF_INPUT,
		.pull_up_en=GPIO_PULLUP_ENABLE,
	};
	gpio_config(&bconfig);
}

#define BM_LEFT 1
#define BM_RIGHT 2
#define BM_START 4
#define BM_PLUNGER 8
static int get_btn_bitmap() {
	int v=0;
	if (!gpio_get_level(BTN_LEFT)) v|=BM_LEFT;
	if (!gpio_get_level(BTN_RIGHT)) v|=BM_RIGHT;
	if (!gpio_get_level(BTN_START)) v|=BM_START;
	if (!gpio_get_level(BTN_PLUNGER)) v|=BM_PLUNGER;
	return v;
}

int old_btns;

int gfx_get_key() {
	const int keys[]={INPUT_LFLIP,INPUT_RFLIP,INPUT_F1,INPUT_SPRING};
	int r=0;
	int new_btns=get_btn_bitmap();
	for (int i=0; i<4; i++) {
		if ((new_btns^old_btns)&(1<<i)) {
			r=keys[i];
			if (old_btns&(1<<i)) r|=INPUT_RELEASE;
		}
	}
	old_btns=new_btns;
	return r;
}


void gfx_show(uint8_t *buf, uint32_t *pal, int h, int w, int scroll) {
	lcd_show(buf, pal, h, w, scroll+100);
}

