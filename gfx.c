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

void gfx_tick() {
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		if (event.type==SDL_QUIT) {
			exit(0);
		}
	}
}

void gfx_show(uint8_t *buf, uint32_t *pal, int w, int h) {
	uint32_t *pixels;
	int pitch;
	int r=SDL_LockTexture(texture, NULL, (void**)&pixels, &pitch);
	for (int y=0; y<h; y++) {
		for (int x=0; x<w; x++) {
			pixels[y*(pitch/4)+x]=pal[buf[y*w+x]];
		}
	}

	SDL_UnlockTexture(texture);
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);
}


static SDL_AudioDeviceID audio_dev_id;

void init_audio(int samprate, audio_cb_t cb) {
	SDL_AudioSpec want={0}, have={0};
	want.freq=samprate;
	want.format=AUDIO_S16SYS;
	want.channels=2;
	want.samples=1024*1;
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
