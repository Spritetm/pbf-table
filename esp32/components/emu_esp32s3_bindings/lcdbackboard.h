spi_device_handle_t lcdbb_add_to_bus(int spi_bus);
void lcdbb_init(spi_device_handle_t spi);
void lcdbb_render_finish();
void lcdbb_handle_dmd(uint8_t *dmdfb, uint32_t *pal);


