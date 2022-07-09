#include <stdint.h>

void initials_handle_vram(uint8_t *vram);
//returns 1 if buttons should not be passed to system
int initials_handle_button(int btn);
int initials_getscancode();
