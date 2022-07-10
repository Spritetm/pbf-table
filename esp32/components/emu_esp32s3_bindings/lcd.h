//Show a VGA video buffer on the main LCD. buf and pal are the buffers for the
//pixel data and palette respectively. h/v indicate the buffers height and width
//in pixels. if show_dmd is one, the dmd bitmap will be shown on the backboard.
void lcd_show(uint8_t *buf, uint32_t *pal, int h, int w, int show_dmd);
//Init LCDs
void lcd_init(void);
//Get the frame number we're showing
int lcd_get_frame();
