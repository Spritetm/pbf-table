#include <stdint.h>
//this is what's indicated as 'togglaren' in the sources
typedef struct __attribute__((packed)) {
	uint8_t s_balls; //3 (0) or 5 (1). Default is 3
	uint8_t s_angle; //high (0) or low (1). Default is high.
	uint8_t s_scrolling; //medium (1), soft (2), hard (0). Default is medium.
	uint8_t s_im; //ingame music, on (0) or off (1). Default is on.
	uint8_t s_resolution; //reso, normal (0) or high (1). Default is normal.
	uint8_t s_mode; //color mode, color (0) or mono (1). Default is color.
} pref_type_pbf_prefs_t;

//Main prefs struct. Stores Pinball Fantasies prefs as well as stuff for the emulator.
typedef struct __attribute__((packed)) {
	int plunger_min;
	int plunger_max;
	pref_type_pbf_prefs_t pbf_prefs;
} pref_type_t;

void prefs_read(pref_type_t *p);
void prefs_write(pref_type_t *p);

#define PREFS_DEFAULT { 	\
	.pbf_prefs={			\
		.s_balls=0,			\
		.s_angle=0,			\
		.s_scrolling=1,		\
		.s_im=0,			\
		.s_resolution=0,	\
		.s_mode=0			\
	},						\
	.plunger_min=500,		\
	.plunger_max=1000,		\
}

