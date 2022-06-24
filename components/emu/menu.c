#include <stdlib.h>
#include <assert.h>
#include "backboard.h"
#include "gfx.h"
#include "haptic.h"
#include "music.h"
#include "emu.h"
#include "mmap_file.h"
#include "esp_log.h"

#define TAG "menu"

static void show_table_fade(const uint8_t *vd, int table, int fade) {
	const int vram_sz=256*1024;
	const int pal_sz=1024;
	uint8_t *vram=(uint8_t*)&vd[(vram_sz+pal_sz)*table];
	uint32_t *pal=(uint32_t*)&vd[(vram_sz+pal_sz)*table+vram_sz];
	uint32_t palfaded[256];
	for (int i=0; i<256; i++) {
		int r=(pal[i]>>16)&0xff;
		int g=(pal[i]>>8)&0xff;
		int b=(pal[i]>>0)&0xff;
		r=(r*fade)/256;
		g=(g*fade)/256;
		b=(b*fade)/256;
		palfaded[i]=(r<<16)+(g<<8)+(b);
	}
	gfx_show(vram, palfaded, 336, 607, 0);
}

static void show_table(const uint8_t *vd, int table, int fade_dir) {
	int fade;
	if (fade_dir<0) {
		fade=255;
		fade_dir=-8;
	} else {
		fade=1;
		fade_dir=8;
	}
	while (fade>=0 && fade<=256) {
		while(!gfx_frame_done());
		show_table_fade(vd, table, fade);
		fade+=fade_dir;
	}
}

const char *prgs[4]={"TABLE1.PRG", "TABLE2.PRG", "TABLE3.PRG", "TABLE4.PRG"};
const char *mods[4]={"TABLE1.MOD", "TABLE2.MOD", "TABLE3.MOD", "TABLE4.MOD"};

void menu_start() {
	const uint8_t *vramsnapshots;
	gfx_init();
	mmap_file("table-screens.bin", (const void**)&vramsnapshots);
	music_init();
	haptic_init();
	int r=music_load("INTRO.MOD");
	assert(r);
	
	int table=0;
	show_table(vramsnapshots, 0, 1);
	backboard_show(BBIMG_TABLE(0));
	ESP_LOGI(TAG, "In main menu");
	while(1) {
		int k=gfx_get_key();
		if (k==INPUT_F1) {
			ESP_LOGI(TAG, "Running table %d", table);
			music_unload();
			emu_run(prgs[table], mods[table]);
		} else if (k==INPUT_LFLIP) {
			backboard_show(BBIMG_TABLE((table-1)&3));
			show_table(vramsnapshots, table, -1);
			table=(table-1)&3;
			show_table(vramsnapshots, table, 1);
		} else if (k==INPUT_RFLIP) {
			backboard_show(BBIMG_TABLE((table+1)&3));
			show_table(vramsnapshots, table, -1);
			table=(table+1)&3;
			show_table(vramsnapshots, table, 1);
		}
	}
}