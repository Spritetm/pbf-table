
#define HAPTIC_EVT_LFLIPPER 0
#define HAPTIC_EVT_RFLIPPER 1
#define HAPTIC_EVT_BALL 2

void haptic_init();
//Note: accel_x and accel_y are -128..127
void haptic_event(int event_type, int accel_x, int accel_y);
