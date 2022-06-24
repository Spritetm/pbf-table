#include "driver/i2c.h"
#include "driver/adc.h"
#include "esp_log.h"
#include "io.h"

#define TAG "io"

#define GPIO_SDA 44 //=rxd0
#define GPIO_SCL 1
#define PLUNGER_ADC ADC1_CHANNEL_1

#define I2C_PORT 0
//#define TCA_ADR 0x27
#define TCA_ADR 0x20

#define BTN_LEFT 6
#define BTN_START 5
#define BTN_RIGHT 7
#define LCD_RESET 4

static int out_data=0;

void io_init() {
	i2c_config_t conf = {
		.mode = I2C_MODE_MASTER,
		.sda_io_num = GPIO_SDA,
		.sda_pullup_en = GPIO_PULLUP_ENABLE,
		.scl_io_num = GPIO_SCL,
		.scl_pullup_en = GPIO_PULLUP_ENABLE,
		.master.clk_speed = 100*1000,
	};
	ESP_ERROR_CHECK(i2c_param_config(I2C_PORT, &conf));
	ESP_ERROR_CHECK(i2c_driver_install(I2C_PORT, I2C_MODE_MASTER, 0, 0, 0));

	//Configure outputs
	uint8_t w[2];
	w[0]=0x03; //config
	w[1]=(1<<BTN_LEFT)|(1<<BTN_RIGHT)|(1<<BTN_START);
	ESP_ERROR_CHECK(i2c_master_write_to_device(I2C_PORT, TCA_ADR, w, 2, pdMS_TO_TICKS(100)));
	w[0]=1; //output
	w[1]=0;
	ESP_ERROR_CHECK(i2c_master_write_to_device(I2C_PORT, TCA_ADR, w, 2, pdMS_TO_TICKS(100)));

	ESP_ERROR_CHECK(adc1_config_channel_atten(PLUNGER_ADC, ADC_ATTEN_DB_2_5));
	ESP_ERROR_CHECK(adc1_config_width(ADC_WIDTH_BIT_12));
	ESP_LOGI(TAG, "IO inited");
}

void io_lcd_reset(int reset) {
	if (reset) out_data|=(1<<LCD_RESET); else out_data&=~(1<<LCD_RESET);
	uint8_t data[2]={1, out_data};
	ESP_ERROR_CHECK(i2c_master_write_to_device(I2C_PORT, TCA_ADR, data, 2, pdMS_TO_TICKS(100)));
}

int io_get_btn_bitmap() {
	uint8_t w=0, r=0;
	ESP_ERROR_CHECK(i2c_master_write_read_device(I2C_PORT, TCA_ADR, &w, 1, &r, 1, pdMS_TO_TICKS(100)));

	int v=0;
	if (!(r&(1<<BTN_LEFT))) v|=BM_LEFT;
	if (!(r&(1<<BTN_RIGHT))) v|=BM_RIGHT;
	if (!(r&(1<<BTN_START))) v|=BM_START;
	return v;
}

int s_last_adc_out=4096;

int io_get_plunger() {
	int out;
	int r=0;
	out=adc1_get_raw(PLUNGER_ADC);
	if (out > s_last_adc_out+700) {
		r=((out-500)*32)/1000;
		if (r>32) r=32;
		printf("Plunger: %d --> %d\n", out, r);
	}
	s_last_adc_out=out;
	return r;
}
