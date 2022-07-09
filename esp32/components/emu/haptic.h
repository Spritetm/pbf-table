//Bindings for haptic actuators
/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
 * this notice you can do whatever you want with this stuff. If we meet some day, 
 * and you think this stuff is worth it, you can buy me a beer in return. 
 * ----------------------------------------------------------------------------
 */

#define HAPTIC_EVT_LFLIPPER 0
#define HAPTIC_EVT_RFLIPPER 1
#define HAPTIC_EVT_BALL 2

void haptic_init();
//Note: accel_x and accel_y are -128..127
void haptic_event(int event_type, int accel_x, int accel_y);
