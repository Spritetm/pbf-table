
//initialize IO
void io_init();

//set reset line to both LCDs
void io_lcd_reset(int reset);

//get bitmap of buttons pressed
#define BM_LEFT 1
#define BM_RIGHT 2
#define BM_START 4
#define BM_PLUNGER 8
#define BM_TILT 16
int io_get_btn_bitmap();

//get raw plunger value
int io_get_plunger();
