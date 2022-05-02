#include "stdint.h"
#include <SDL2/SDL.h>
#include "gfx.h"

SDL_Window *window;
SDL_Renderer *renderer;
SDL_Texture *texture;

#define TEX_WIDTH 320
#define TEX_HEIGHT 607

#define SCREEN_WIDTH TEX_WIDTH*2
#define SCREEN_HEIGHT TEX_HEIGHT*2


void gfx_init() {
	if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
		printf("Error SDL init\n");
		exit(1);
	}
	window=SDL_CreateWindow("Pinball Fantasies", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
	if (window == NULL) {
		printf("Error SDL setvideomode\n");
		exit(1);
	}
	renderer=SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
	if (renderer == NULL) {
		printf("Error SDL renderer\n");
		exit(1);
	}
	texture=SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, TEX_WIDTH, TEX_HEIGHT);
}

//Keep in sync with input_*
static int keys[]={
	0,
	SDLK_DOWN,
	SDLK_LSHIFT,
	SDLK_RSHIFT,
	SDLK_SPACE,
	SDLK_F1,
	SDLK_F2,
	SDLK_F3,
	SDLK_F4,
	SDLK_t,
	-1
};

int gfx_get_key() {
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		if (event.type==SDL_QUIT) {
			exit(0);
		} else if (event.type==SDL_KEYDOWN || event.type==SDL_KEYUP) {
			for (int i=1; keys[i]>=0; i++) {
				if (event.key.keysym.sym==keys[i]) {
					return i|((event.type==SDL_KEYUP)?INPUT_RELEASE:0);
				}
			}
		}
	}
	return -1;
}

void gfx_show(uint8_t *buf, uint32_t *pal, int w, int h, int scroll) {
	uint32_t *pixels=0;
	int pitch;
	int texh, texw;
	SDL_QueryTexture(texture, NULL, NULL, &texw, &texh);
	int r=SDL_LockTexture(texture, NULL, (void**)&pixels, &pitch);
	if (r!=0) {
		printf("SDL_LockTexture: %s\n", SDL_GetError());
		return;
	}
	int in_pitch=w;
	if (h>texh) h=texh;
	if (w>texw) w=texw;
	for (int y=0; y<h; y++) {
		for (int x=0; x<w; x++) {
			pixels[y*(pitch/4)+x]=pal[buf[y*in_pitch+x]];
		}
	}

	SDL_UnlockTexture(texture);
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);
}


static SDL_AudioDeviceID audio_dev_id;

void audio_init(int samprate, audio_cb_t cb) {
	SDL_AudioSpec want={0}, have={0};
	want.freq=samprate;
	want.format=AUDIO_S16SYS;
	want.channels=1;
	want.samples=1024;
	want.callback=cb;
	audio_dev_id=SDL_OpenAudioDevice(NULL, 0, &want, &have, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
	if (audio_dev_id<=0) {
		printf("Can't initialize sdl audio\n");
		exit(1);
	}
	SDL_PauseAudioDevice(audio_dev_id, 0);
}

void audio_lock() {
	SDL_LockAudioDevice(audio_dev_id);
}

void audio_unlock() {
	SDL_UnlockAudioDevice(audio_dev_id);
}

int gfx_frame_done() {
	return 1;
}
