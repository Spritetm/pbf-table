//Bindings for graphics output of pbf emu
/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
 * this notice you can do whatever you want with this stuff. If we meet some day, 
 * and you think this stuff is worth it, you can buy me a beer in return. 
 * ----------------------------------------------------------------------------
 */

#include <stdint.h>

//Keys as returned by gfx_get_key
#define INPUT_SPRING 1
#define INPUT_LFLIP 2
#define INPUT_RFLIP 3
#define INPUT_TILT 4
#define INPUT_F1 5
#define INPUT_F2 6
#define INPUT_F3 7
#define INPUT_F4 8
#define INPUT_TEST 9
#define INPUT_RELEASE 0x80 //or with this for a release event
#define INPUT_RAWSCANCODE 0x100 //or any number with this to interpret lower 8 bits as a raw scancode

//Initialize gfx.
void gfx_init();
//Get a key event. Returns INPUT_*, possubly ORred with INPUT_RELEASE for a release event.
int gfx_get_key();
//Get raw plunger hardware value. Should return a higher number when plunger is pulled back harder and released
int gfx_get_plunger();
//Show graphics on main LCD. buf contains the VRAM, pal the r8g8b8 palette, h and w the size of buf in pixels,
//scroll the offset in vram to show (if shown on a smaller screen)
void gfx_show(uint8_t *buf, uint32_t *pal, int h, int w, int scroll);
//Returns 1 if the frame sent by gfx_show is sent, 0 otherwise
int gfx_frame_done();
//Wait until the frame sent by gfx_show is shown.
//Used in menu as a delay routine, no need to be precise
void gfx_wait_frame_done(); 
//Enable/disable showing the DMD on the backbox
void gfx_enable_dmd(int enable); 

//Callback for audio. This callback should fill the stream buffer fully.
typedef void(*audio_cb_t)(void* userdata, uint8_t* stream, int len);

//Initialize the audio according to the given sample rate using the given
//callback as an audio source.
void audio_init(int samprate, audio_cb_t cb);

//Lock/unlock the audio callback. Between these calls, it's guaranteed
//that the audio callback will not be running.
void audio_lock();
void audio_unlock();
