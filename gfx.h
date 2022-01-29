void gfx_init();
void gfx_tick();
void gfx_show(uint8_t *buf, uint32_t *pal, int h, int w);

typedef void(*audio_cb_t)(void* userdata, uint8_t* stream, int len);
void init_audio(int samprate, audio_cb_t cb);
void audio_lock();
void audio_unlock();