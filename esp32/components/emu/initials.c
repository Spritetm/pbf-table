//Code to handle entering high score initials without using a keyboard
/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
 * this notice you can do whatever you want with this stuff. If we meet some day, 
 * and you think this stuff is worth it, you can buy me a beer in return. 
 * ----------------------------------------------------------------------------
 */

#include <stdint.h>
#include <stdio.h>
#include "font.h"
#include "gfx.h"

/*
This is code to handle inputting your initials when you've gotten a high score. Real-life
pinball machines allow you to select a letter using the flipper buttons and 'start' to
go to the next letter. Pinball Fantasies is a PC game, however, so it simply uses presses
of letters on the keyboard to get initials. We don't have a keyboard in our hardware 
implementation, so we need to fake it.

The way we do this is by detecting if the DMD shows the 'HIGHSCORE (   )' screen. If so,
we overlay our own character (one which is modifyable with the left and right flipper
button) over the DMD VRAM. When the player presses the start button (F1 key), this code
sends the scancode of the corresponding button to the emulator and advances to the next
digit.
*/


//Bitmap to match against video ram. 0 = pixel off, 1 = pixel on, 2 = don't care
const uint8_t hiscore_bitmap[]={
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,0,0,0,0,0,0,0,
1,1,1,0,1,1,1,0,1,1,1,1,1,1,1,0,0,1,1,1,1,1,0,0,1,1,1,0,1,1,1,0,0,1,1,1,1,1,1,0,0,1,1,1,1,1,0,0,0,1,1,1,1,1,0,0,1,1,1,1,1,1,0,0,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,1,1,0,0,0,0,0,
1,1,1,0,1,1,1,0,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,0,1,1,1,0,1,1,1,0,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,1,1,1,0,0,0,0,
1,1,1,0,1,1,1,0,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,0,1,1,1,0,1,1,1,0,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,0,1,1,1,0,0,0,
1,1,1,0,1,1,1,0,0,0,1,1,1,0,0,0,1,1,1,0,1,1,1,0,1,1,1,0,1,1,1,0,1,1,1,0,0,0,0,0,1,1,1,0,1,1,1,0,1,1,1,0,1,1,1,0,1,1,1,0,1,1,1,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,1,1,1,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,0,0,1,1,1,0,0,
1,1,1,0,1,1,1,0,0,0,1,1,1,0,0,0,1,1,1,0,1,1,1,0,1,1,1,0,1,1,1,0,1,1,1,0,0,0,0,0,1,1,1,0,1,1,1,0,1,1,1,0,1,1,1,0,1,1,1,0,1,1,1,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,1,1,1,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,0,0,1,1,1,0,0,
1,1,1,1,1,1,1,0,0,0,1,1,1,0,0,0,1,1,1,0,0,0,0,0,1,1,1,1,1,1,1,0,1,1,1,1,1,1,0,0,1,1,1,0,0,0,0,0,1,1,1,0,1,1,1,0,1,1,1,1,1,1,1,0,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,0,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,0,0,0,1,1,1,0,
1,1,1,1,1,1,1,0,0,0,1,1,1,0,0,0,1,1,1,0,1,1,1,0,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,0,1,1,1,0,0,0,0,0,1,1,1,0,1,1,1,0,1,1,1,1,1,1,0,0,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,0,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,0,0,0,1,1,1,0,
1,1,1,1,1,1,1,0,0,0,1,1,1,0,0,0,1,1,1,0,1,1,1,0,1,1,1,1,1,1,1,0,0,1,1,1,1,1,1,0,1,1,1,0,0,0,0,0,1,1,1,0,1,1,1,0,1,1,1,1,1,1,1,0,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,0,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,0,0,0,1,1,1,0,
1,1,1,0,1,1,1,0,0,0,1,1,1,0,0,0,1,1,1,0,0,1,1,0,1,1,1,0,1,1,1,0,0,0,0,0,1,1,1,0,1,1,1,0,1,1,1,0,1,1,1,0,1,1,1,0,1,1,1,0,1,1,1,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,0,0,1,1,1,0,0,
1,1,1,0,1,1,1,0,0,0,1,1,1,0,0,0,1,1,1,0,0,1,1,0,1,1,1,0,1,1,1,0,0,0,0,0,1,1,1,0,1,1,1,0,1,1,1,0,1,1,1,0,1,1,1,0,1,1,1,0,1,1,1,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,0,0,1,1,1,0,0,
1,1,1,0,1,1,1,0,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,0,1,1,1,0,1,1,1,0,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,0,1,1,1,0,1,1,1,0,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,0,1,1,1,0,0,0,
1,1,1,0,1,1,1,0,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,0,1,1,1,0,1,1,1,0,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,0,1,1,1,0,1,1,1,0,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,1,1,1,0,0,0,0,
1,1,1,0,1,1,1,0,1,1,1,1,1,1,1,0,0,1,1,1,1,1,1,0,1,1,1,0,1,1,1,0,1,1,1,1,1,1,0,0,0,1,1,1,1,1,0,0,0,1,1,1,1,1,0,0,1,1,1,0,1,1,1,0,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,1,1,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,0,0,0,0,0,0,0,
};

#define CHARCOUNT 27
//const char *font_chars="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ?()- ";

//We only accept A-Z and space
const int fontpos[]={10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
	22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 40};

//Scancodes for A-Z and space.
const uint8_t scancodes[]={
	0x1e, 0x30, 0x2e, 0x20, 0x12, 0x21, 0x22, 0x23, 0x17, 0x24, 
	0x25, 0x26, 0x32, 0x31, 0x18, 0x19, 0x10, 0x13, 0x1f, 0x14,
	0x16, 0x2F, 0x11, 0x2D, 0x15, 0x2C, 0x39};

#define POS_WAITDONE -2
#define POS_RESTART -1

static int hiscore_charpos=POS_RESTART;
static int hiscore_curchar=0;
static int scancode=POS_RESTART;

void initials_handle_vram(uint8_t *vram) {
	//Check if vram shows hiscore. We do that by comparing VRAM to the known
	//hiscore bitmap.
	//Note that we don't know the palette for 'light' and 'dark', so we find that on the fly.
	int pal[2]={-1, -1};
	const uint8_t *hp=hiscore_bitmap;
	int in_highscore=1;
	for (int y=2; y<32; y+=2) {
		for (int x=0; x<320; x+=2) {
			int px=vram[y*336+x];
			if (*hp!=2) { //if we care about this pixel
				if (px!=pal[*hp]) {
					if (pal[*hp]==-1) {
						//We don't know the palette index of this type of DMD pixel. Record.
						pal[*hp]=px;
						if (pal[0]==pal[1]) {
							//'Light' and 'dark' palette should be different; if not, we're not at the hiscore screen.
							in_highscore=0;
							break;
						}
					} else {
						//No match.
						in_highscore=0;
						break;
					}
				}
			}
			hp++;
		}
		if (!in_highscore) break;
	}
	if (!in_highscore) {
		//Not on highscore screen. Abort out.
		hiscore_charpos=POS_RESTART;
		return;
	}
	if (hiscore_charpos==POS_WAITDONE) {
		//We entered the initials for this highscore already, but the screen is
		//still showing. wait till the emu processed it.
		return;
	}
	if (hiscore_charpos==POS_RESTART) {
		//Just entered hiscore initials enter screen. Reset state machine.
		hiscore_charpos=0;
		hiscore_curchar=0;
	}
	//Overlay selected character at the charpos position.
	//We directly modify the DMD pixels in VRAM.
	int of=hiscore_charpos*16+256;
	const uint8_t *fontp=&pf_dmd_font[fontpos[hiscore_curchar]*13];
	for (int y=4; y<30; y+=2) {
		int ch=*fontp++;
		for (int x=of; x<of+16; x+=2) {
			vram[y*336+x]=pal[(ch&0x80)?1:0];
			ch<<=1;
		}
	}
}

//Handle buttons used to modify high score.
int initials_handle_button(int btn) {
	if (hiscore_charpos==-1) return 0; //not in hiscore entry mode
	if (btn==INPUT_LFLIP) {
		hiscore_curchar--;
		if (hiscore_curchar<0) hiscore_curchar=CHARCOUNT-1;
	} else if (btn==INPUT_RFLIP) {
		hiscore_curchar++;
		if (hiscore_curchar>=CHARCOUNT) hiscore_curchar=0;
	} else if (btn==(INPUT_F1|INPUT_RELEASE)) {
		scancode=scancodes[hiscore_curchar];
		hiscore_charpos++;
		if (hiscore_charpos==3) hiscore_charpos=POS_WAITDONE;
	}
	return 1; //returning 1 makes the emulator discard the keypress
}

int initials_getscancode() {
	int r=scancode; //returned scancode
	if ((scancode&0x80)==0) {
		//Send the same scancode with a break code next.
		scancode|=0x80;
	} else {
		//sent the break previously; we're done
		scancode=-1;
	}
	return r;
}
