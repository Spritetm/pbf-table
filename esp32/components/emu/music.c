//Audio handler
/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
 * this notice you can do whatever you want with this stuff. If we meet some day, 
 * and you think this stuff is worth it, you can buy me a beer in return. 
 * ----------------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "ibxm/ibxm.h"
#include "mmap_file.h"
#include <assert.h>
#include "music.h"
#include "gfx.h"

static struct module *music_module;
static struct replay *music_replay=NULL;
static int *music_mixbuf;
static int music_mixbuf_len;
static int music_mixbuf_left;
static const void *mmapped_module;
static int volume;


#define SAMP_RATE 22000


static void fill_stream_buf(int16_t *stream, int *mixbuf, int len) {
	for (int i=0; i<len; i++) {
		int v=(mixbuf[i]*volume)/256;
		if (v<-32768) v=-32768;
		if (v>32767) v=32767;
		stream[i]=v;
	}
}

static void audio_cb(void* userdata, uint8_t* streambytes, int len) {
	if (volume==0 || music_replay==NULL) {
		//audio disabled. Output an all-zeroes stream.
		memset(streambytes, 0, len);
		return;
	}
	int16_t *stream=(int16_t*)streambytes;
	len=len/2; //because bytes -> words
	int pos=0;
	if (len==0) return;
	//if there's still music in the buffer, use that
	if (music_mixbuf_left!=0) {
		pos=music_mixbuf_left;
		fill_stream_buf(stream, &music_mixbuf[music_mixbuf_len-music_mixbuf_left], music_mixbuf_left);
	}
	music_mixbuf_left=0;
	//Refull until music buffer is full.
	while (pos!=len) {
		music_mixbuf_len=replay_get_audio(music_replay, (int*)music_mixbuf);
		int cplen=music_mixbuf_len;
		if (pos+cplen>len) {
			cplen=len-pos;
			music_mixbuf_left=music_mixbuf_len-cplen;
		}
		fill_stream_buf(&stream[pos], music_mixbuf, cplen);
		pos+=cplen;
	}
}

void music_init() {
	audio_init(SAMP_RATE, audio_cb);
}

int music_load(const char *fname) {
	audio_lock();
	struct data mod_data;
	mod_data.length=mmap_file(fname, &mmapped_module);
	mod_data.buffer=(void*)mmapped_module;

	char msg[64];
	music_module=module_load(&mod_data, msg);
	if (music_module==NULL) {
		printf("%s: %s\n", fname, msg);
		audio_unlock();
		return 0;
	}
	music_replay=new_replay(music_module, SAMP_RATE, 0);
	assert(music_replay);
	music_mixbuf=malloc(calculate_mix_buf_len(SAMP_RATE)*4);
	assert(music_mixbuf);
	music_mixbuf_left=0;
	music_mixbuf_len=0;
	volume=256;
	audio_unlock();
	return 1;
}

void music_unload() {
	audio_lock();
	volume=0;
	dispose_replay(music_replay);
	music_replay=NULL;
	dispose_module(music_module);
	music_module=NULL;
//	munmap_file(mmapped_module); //<= this breaks it
	mmapped_module=NULL;
	free(music_mixbuf);
	music_mixbuf=NULL;
	audio_unlock();
}

int music_get_sequence_pos() {
	audio_lock();
	int r=replay_get_sequence_pos(music_replay);
	audio_unlock();
	return r;
}

void music_set_sequence_pos(int pos) {
	audio_lock();
	replay_set_sequence_pos(music_replay, pos);
	audio_unlock();
}

void music_sfxchan_play(int a, int b, int c) {
	audio_lock();
	sfxchan_play(music_replay, a, b, c);
	audio_unlock();
}

int music_has_looped() {
	audio_lock();
	int r=has_looped(music_replay);
	audio_unlock();
	return r;
}

