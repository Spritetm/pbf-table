void io_init();
void io_lcd_reset(int reset);
#define BM_LEFT 1
#define BM_RIGHT 2
#define BM_START 4
#define BM_PLUNGER 8
int io_get_btn_bitmap();
int io_get_plunger();
