//Attack backboard LCD to an initialized SPI bus
spi_device_handle_t lcdbb_add_to_bus(int spi_bus);
//Initialize the (attached) backboard LCD
void lcdbb_init(spi_device_handle_t spi);
//Show the DMD portion of VGA memory on the backbox LCD
void lcdbb_handle_dmd(uint8_t *dmdfb, uint32_t *pal);

//Note backboard.c also implements backboard_show from emu/backboard.h

