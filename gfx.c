#include "stdint.h"
#include <SDL2/SDL.h>

SDL_Window *window;
SDL_Renderer *renderer;
SDL_Texture *texture;

#define TEX_WIDTH 320
#define TEX_HEIGHT 600

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
	int shown[255]={0};
	for (int y=0; y<h; y++) {
		for (int x=0; x<w; x++) {
			pixels[y*(pitch/4)+x]=pal[buf[y*w+x]];
			if (shown[buf[y*w+x]]==0 && y<32) {
				printf("col %06X\n", pal[buf[y*w+x]]);
				shown[buf[y*w+x]]=1;
			}
		}
	}

	SDL_UnlockTexture(texture);
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);
}
