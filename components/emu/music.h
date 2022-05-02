void music_init();
int music_load(const char *fname);
void music_unload();
int music_get_sequence_pos();
void music_set_sequence_pos(int pos);
void music_sfxchan_play(int a, int b, int c);
int music_has_looped();
