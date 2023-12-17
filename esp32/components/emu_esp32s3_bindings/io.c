//IO handlers, specifically for the things connected to the I2C port expander and
//the analog plunger.
/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
 * this notice you can do whatever you want with this stuff. If we meet some day, 
 * and you think this stuff is worth it, you can buy me a beer in return. 
 * ----------------------------------------------------------------------------
 */

#include "driver/i2c.h"
#include "driver/adc.h"
#include "esp_log.h"
#include "io.h"
#include "mpu6050.h"

#define TAG "io"

#define GPIO_SDA 44 //=rxd0
#define GPIO_SCL 1
#define PLUNGER_ADC ADC1_CHANNEL_1

#define I2C_PORT 0

#define BTN_LEFT 6
#define BTN_START 5
#define BTN_RIGHT 7
#define LCD_RESET 4

static int out_data=0;

static int tca_addr=0;

void io_init() {
	i2c_config_t conf = {
		.mode = I2C_MODE_MASTER,
		.sda_io_num = GPIO_SDA,
		.sda_pullup_en = GPIO_PULLUP_ENABLE,
		.scl_io_num = GPIO_SCL,
		.scl_pullup_en = GPIO_PULLUP_ENABLE,
		.master.clk_speed = 400*1000,
	};
	ESP_ERROR_CHECK(i2c_param_config(I2C_PORT, &conf));
	ESP_ERROR_CHECK(i2c_driver_install(I2C_PORT, I2C_MODE_MASTER, 0, 0, 0));

	//Configure outputs
	uint8_t w[2];
	w[0]=0x03; //config
	w[1]=(1<<BTN_LEFT)|(1<<BTN_RIGHT)|(1<<BTN_START);
	
	//Try to auto-detect tca9534 and tca9534a chips
	const int tca_addrs[]={0x20, 0x38, 0};
	esp_err_t r=ESP_OK;
	int i=0;
	do {
		//find next possible i2c adddress
		tca_addr=tca_addrs[i];
		if (tca_addr==0) {	//ran out of possible addresses
			//bail out
			ESP_LOGE(TAG, "No working TCA9534 detected!");
			ESP_ERROR_CHECK(r);
			break; //just in case
		}
		//see if it acks here
		r=i2c_master_write_to_device(I2C_PORT, tca_addr, w, 2, pdMS_TO_TICKS(100));
		i++;
	} while (r!=ESP_OK);
	//Okay, tca_addr should contain a working address here.

	w[0]=1; //output
	w[1]=0;
	ESP_ERROR_CHECK(i2c_master_write_to_device(I2C_PORT, tca_addr, w, 2, pdMS_TO_TICKS(100)));
	mpu6050_init(I2C_PORT);

	ESP_ERROR_CHECK(adc1_config_channel_atten(PLUNGER_ADC, ADC_ATTEN_DB_2_5));
	ESP_ERROR_CHECK(adc1_config_width(ADC_WIDTH_BIT_12));
	ESP_LOGI(TAG, "IO inited");
}

void io_lcd_reset(int reset) {
	if (reset) out_data|=(1<<LCD_RESET); else out_data&=~(1<<LCD_RESET);
	uint8_t data[2]={1, out_data};
	ESP_ERROR_CHECK(i2c_master_write_to_device(I2C_PORT, tca_addr, data, 2, pdMS_TO_TICKS(100)));
}

int io_get_btn_bitmap() {
	uint8_t w=0, r=0;
	ESP_ERROR_CHECK(i2c_master_write_read_device(I2C_PORT, tca_addr, &w, 1, &r, 1, pdMS_TO_TICKS(100)));

	int v=0;
	if (!(r&(1<<BTN_LEFT))) v|=BM_LEFT;
	if (!(r&(1<<BTN_RIGHT))) v|=BM_RIGHT;
	if (!(r&(1<<BTN_START))) v|=BM_START;

	mpu6050_accel_tp accel[128];
	int n=mpu6050_read_fifo(I2C_PORT, accel, 128);
	int max=0;
	for (int i=0; i<n; i++) {
		if (accel[i].accely < max) max=accel[i].accely;
	}
	//fwiw, 1g is about 2000
	if (max<-3000) {
		v|=BM_TILT;
		printf("TILT!\n");
	}

	return v;
}


int io_get_plunger() {
	return adc1_get_raw(PLUNGER_ADC);
}
