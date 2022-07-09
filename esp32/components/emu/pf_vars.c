//Code to find and interact with variables inside Pinball Fantasies
/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
 * this notice you can do whatever you want with this stuff. If we meet some day, 
 * and you think this stuff is worth it, you can buy me a beer in return. 
 * ----------------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "cpu_addr_space.h"

typedef struct __attribute__((packed)) {
	uint16_t hitvalue;
	uint16_t materialbit;
	uint16_t rotation;
	uint16_t sc_x, sc_y; //165 and 450 on startup
	uint32_t x_pos;
	uint32_t y_pos;
	int16_t x_hast;
	int16_t y_hast;
	uint16_t exahitx;
	uint16_t exahity;
	uint16_t gravy, gravx; //8 and 0 on startup
	uint16_t oldpos, oldshift;
	uint16_t oldx, oldy; //oldy is 19?
} pf_ball_vars_t;


typedef struct __attribute__((packed)) {
	uint8_t allowflip;
	uint8_t first_time_you_fool; //255
	uint8_t balls[12];
	uint8_t no_of_balls; //1
	uint8_t scorechanged;
	uint8_t special_score_changed;
	uint8_t i_uyskjut; //255
	uint8_t behovs_provad, specialmode;
	uint8_t addplayers; //255
} pf_misc_vars_t;

//These are the values of the structs after the exe is loaded but hasn't executed yet.
static const pf_ball_vars_t ball_vars_initial={
	.sc_x=165,
	.sc_y=450,
	.gravy=8,
	.oldy=0x19
};

static const pf_misc_vars_t misc_vars_initial={
	.first_time_you_fool=255,
	.no_of_balls=1,
	.i_uyskjut=255,
	.addplayers=255,
};

//actually the plastic material def
static const uint16_t springpos_hook[]={
	10000,10000/4,400,-500,38,0,0,0
};

static int pf_ball_vars_pos=0;
static int pf_misc_vars_pos=0;
static int pf_springpos_var_pos=0;

#define SEARCH_CHUNK 128 //should be close to 2x the largest struct to look for
void pf_vars_init(int offset_start, int len) {
	uint8_t mem[SEARCH_CHUNK];
	int p=offset_start; //p is offset where mem[0] points to
	for (int i=0; i<SEARCH_CHUNK; i++) {
		mem[i]=cpu_addr_space_read8(p+i);
	}
	while (p<offset_start+len) {
		for (int i=0; i<SEARCH_CHUNK/2; i++) {
			if (memcmp(&mem[i], &ball_vars_initial, sizeof(ball_vars_initial))==0) {
				pf_ball_vars_pos=p+i;
				printf("Found pf_ball_vars_pos: %x\n", pf_ball_vars_pos);
			}
			if (memcmp(&mem[i], &misc_vars_initial, sizeof(misc_vars_initial))==0) {
				pf_misc_vars_pos=p+i;
				printf("Found pf_misc_vars_pos: %x\n", pf_misc_vars_pos);
			}
			if (memcmp(&mem[i], springpos_hook, 16)==0) {
				pf_springpos_var_pos=p+i+16+10;
				printf("Found pf_springpos_var_pos: %x\n", pf_springpos_var_pos);
			}
			if (pf_ball_vars_pos!=0 && pf_misc_vars_pos!=0) break; //found
		}
		memmove(&mem[0], &mem[SEARCH_CHUNK/2], SEARCH_CHUNK/2);
		p+=SEARCH_CHUNK/2;
		for (int i=0; i<SEARCH_CHUNK/2; i++) {
			mem[(SEARCH_CHUNK/2)+i]=cpu_addr_space_read8((SEARCH_CHUNK/2)+p+i);
		}
	}
}

void pf_vars_set_springpos(uint8_t springpos) {
	cpu_addr_space_write8(pf_springpos_var_pos, springpos);
}

int pf_vars_get_flip_enabled() {
	return cpu_addr_space_read8(pf_misc_vars_pos+offsetof(pf_misc_vars_t, allowflip));
}

void pf_vars_get_ball_speed(int *x, int *y) {
	int16_t xx, yy;
	xx=cpu_addr_space_read8(pf_ball_vars_pos+offsetof(pf_ball_vars_t, x_hast));
	xx|=cpu_addr_space_read8(pf_ball_vars_pos+offsetof(pf_ball_vars_t, x_hast)+1)<<8;
	yy=cpu_addr_space_read8(pf_ball_vars_pos+offsetof(pf_ball_vars_t, y_hast));
	yy|=cpu_addr_space_read8(pf_ball_vars_pos+offsetof(pf_ball_vars_t, y_hast)+1)<<8;
	*x=xx; *y=yy;
}
