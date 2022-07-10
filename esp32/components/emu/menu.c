//Code to show the initial menu allowing to select a table
/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
 * this notice you can do whatever you want with this stuff. If we meet some day, 
 * and you think this stuff is worth it, you can buy me a beer in return. 
 * ----------------------------------------------------------------------------
 */

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "backboard.h"
#include "gfx.h"
#include "haptic.h"
#include "music.h"
#include "emu.h"
#include "mmap_file.h"
#include "font.h"
#include "prefs.h"

#define TAG "menu"

//Show a table in flash on the main LCD, but fade-to-black the palette with a given amount.
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

//Fade a table in or out on the main LCD
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

//The program files and audio for the various tables.
const char *prgs[4]={"TABLE1.PRG", "TABLE2.PRG", "TABLE3.PRG", "TABLE4.PRG"};
const char *mods[4]={"TABLE1.MOD", "TABLE2.MOD", "TABLE3.MOD", "TABLE4.MOD"};

static void config_menu();

void menu_start() {
	const uint8_t *vramsnapshots;
	gfx_init();
	gfx_enable_dmd(0);
	mmap_file("table-screens.bin", (const void**)&vramsnapshots);
	music_init();
	haptic_init();
	int r=music_load("INTRO.MOD");
	assert(r);
	
	int table=0;
	show_table(vramsnapshots, 0, 1);
	backboard_show(BBIMG_TABLE(0));
	printf("In main menu\n");
	uint8_t btn[16]={0};
	while(1) {
		//The config menu is shown when lflip+rflip is held and start is pressed. It takes a while
		//because the logic first processes the lflip and rflip events.
		int k=gfx_get_key();
		if (k&0x80) {
			btn[k&15]=0;
		} else {
			btn[k&15]=1;
		}
		if (btn[INPUT_F1] && btn[INPUT_LFLIP] && btn[INPUT_RFLIP]) {
			config_menu();
			show_table(vramsnapshots, table, 1);
			k=0; //so we don't fall through into starting a table
			memset(btn, 0, 16); //so we don't immediately restart the config menu
		}
		if (k==INPUT_F1) {
			printf("Menu: Running table %d\n", table);
			music_unload();
			//technically, we should munmap() the vramsnapshots, but that can corrupt the
			//image while the emu is starting... we'll just 'leak' that for now.
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

const char *font_chars="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ?()-";

//Put a character to VRAM on the given coordinates in the given color.
void config_putchar(uint8_t *vram, int x, int y, char c, int col) {
	int pos=0;
	while (font_chars[pos]!=0 && font_chars[pos]!=c) pos++;
	if (font_chars[pos]==0) return;
	const uint8_t *p=&pf_dmd_font[pos*13];
	for (int yy=0; yy<26; yy+=2) {
		int c=*p;
		for (int xx=0; xx<16; xx+=2) {
			vram[336*(yy+y)+(xx+x)]=(c&0x80)?col:0;
			c<<=1;
		}
		p++;
	}
}

//Put a string to the main LCD at the given text position in the given color.
void config_putstr(uint8_t *vram, int x, int y, const char *str, int col) {
	const char *p=str;
	while (*p!=0) {
		config_putchar(vram, x*16, y*28+32, *p, col);
		x++;
		if (x>25) { //overflow; not used, not sure if that number is correct
			x=0;
			y++;
		}
		p++;
	}
}

//Helper routine to calibrate the plunger. This will get and
//display the maximum plunger value measured, and will return that when
//the start button is pressed.
static int get_plunger_val(uint8_t *vram, uint32_t *pal) {
	//wait till start release
	while (gfx_get_key()!=(INPUT_F1|0x80)) gfx_wait_frame_done();
	config_putstr(vram, 0, 13, "START TO FINISH", 4);
	int max=0;
	while(gfx_get_key()!=INPUT_F1) {
		int p=gfx_get_plunger();
		if (p>max) max=p;
		char buf[16];
		sprintf(buf, "%d", max);
		gfx_wait_frame_done();
		config_putstr(vram, 5, 14, buf, 4);
		gfx_show(vram, pal, 336, 607, 0);
	}
	return max;
}

//Config meny routine
static void config_menu() {
	//Allocate custom fb/pal
	uint8_t *vram=calloc(336*607, sizeof(uint8_t));
	uint32_t *pal=calloc(256, sizeof(uint32_t));
	//Set up palette
	for (int i=0; i<8; i++) {
		pal[i]=((i&1)?0xff0000:0)|((i&2)?0xff00:0)|((i&4)?0xff:0);
	}
	gfx_show(vram, pal, 336, 607, 0);

	int choice=0;
	while(1) {
		pref_type_t prefs=PREFS_DEFAULT;
		prefs_read(&prefs);
		char buf[16];
		memset(vram, 0, 336*607);

		//Show config menu settings
		config_putstr(vram, 0, 0, "CONFIG", 7);
		config_putstr(vram, 1, 2, "BALL COUNT", 4);
		config_putstr(vram, 5, 3, prefs.pbf_prefs.s_balls?"5":"3", 2);
		config_putstr(vram, 1, 4, "ANGLE", 4);
		config_putstr(vram, 5, 5, prefs.pbf_prefs.s_angle?"LOW":"HIGH", 2);
		config_putstr(vram, 1, 6, "PLUNGER LO", 4);
		sprintf(buf, "%d", prefs.plunger_min);
		config_putstr(vram, 5, 7, buf, 2);
		config_putstr(vram, 1, 8, "PLUNGER HI", 4);
		sprintf(buf, "%d", prefs.plunger_max);
		config_putstr(vram, 5, 9, buf, 2);
		config_putstr(vram, 1, 10, "EXIT", 4);

		//selection indicator
		config_putstr(vram, 0, 2+choice*2, "-", 7);
		gfx_show(vram, pal, 336, 607, 0);

		//wait for keypress
		int k;
		while ((k=gfx_get_key())==0) {
			gfx_wait_frame_done();
		}
		//handle keypress
		if (k==INPUT_RFLIP) {
			choice++;
			if (choice>=5) choice=0;
		} else if (k==INPUT_LFLIP) {
			choice--;
			if (choice<0) choice=4;
		} else if (k==INPUT_F1) {
			//change selected setting
			if (choice==0) {
				prefs.pbf_prefs.s_balls^=1;
			} else if (choice==1) {
				prefs.pbf_prefs.s_angle^=1;
			} else if (choice==2) {
				config_putstr(vram, 0, 12, "PULL PLUNGER LEAST", 4);
				prefs.plunger_min=get_plunger_val(vram, pal);
			} else if (choice==3) {
				config_putstr(vram, 0, 12, "PULL PLUNGER MOST", 4);
				prefs.plunger_max=get_plunger_val(vram, pal);
			} else if (choice==4) {
				//exit menu
				break;
			}
			prefs_write(&prefs);
		}
	}
	//Menu should exit. Wait till start button release
	while (gfx_get_key() != (INPUT_F1|0x80)) gfx_wait_frame_done();
	free(vram);
	free(pal);
}
