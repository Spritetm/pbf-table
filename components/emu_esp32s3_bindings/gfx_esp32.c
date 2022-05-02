#include <stdio.h>
#include <esp_log.h>
#include <gfx.h>
#include "lcd.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "cpu_addr_space.h"

#define I2C_PORT 0
#define TCA_ADR 0x27

#define BTN_LEFT 3
#define BTN_START 1
#define BTN_PLUNGER 2
#define BTN_RIGHT 0
#define HAPTIC_A 4
#define HAPTIC_B 5

static SemaphoreHandle_t rdy_sem;

void gfxinit_task(void *arg) {
	lcd_init();
	i2c_config_t conf = {
		.mode = I2C_MODE_MASTER,
		.sda_io_num = 40,
		.sda_pullup_en = GPIO_PULLUP_ENABLE,
		.scl_io_num = 41,
		.scl_pullup_en = GPIO_PULLUP_ENABLE,
		.master.clk_speed = 100*1000,
	};
	ESP_ERROR_CHECK(i2c_param_config(I2C_PORT, &conf));
	ESP_ERROR_CHECK(i2c_driver_install(I2C_PORT, I2C_MODE_MASTER, 0, 0, 0));

	//Configure outputs
	uint8_t w[2];
	w[0]=0x03; //config
	w[1]=(1<<BTN_LEFT)|(1<<BTN_RIGHT)|(1<<BTN_START)|(1<<BTN_PLUNGER);
	ESP_ERROR_CHECK(i2c_master_write_to_device(I2C_PORT, TCA_ADR, w, 2, pdMS_TO_TICKS(100)));
	w[0]=1; //output
	w[1]=0;
	ESP_ERROR_CHECK(i2c_master_write_to_device(I2C_PORT, TCA_ADR, w, 2, pdMS_TO_TICKS(100)));
	xSemaphoreGive(rdy_sem);
	vTaskDelete(NULL);
}

#define BM_LEFT 1
#define BM_RIGHT 2
#define BM_START 4
#define BM_PLUNGER 8
static int get_btn_bitmap() {
	uint8_t w=0, r=0;
	ESP_ERROR_CHECK(i2c_master_write_read_device(I2C_PORT, TCA_ADR, &w, 1, &r, 1, pdMS_TO_TICKS(100)));

	int v=0;
	if (!(r&(1<<BTN_LEFT))) v|=BM_LEFT;
	if (!(r&(1<<BTN_RIGHT))) v|=BM_RIGHT;
	if (!(r&(1<<BTN_START))) v|=BM_START;
	if (!(r&(1<<BTN_PLUNGER))) v|=BM_PLUNGER;
	return v;
}


void gfx_init() {
	rdy_sem=xSemaphoreCreateBinary();
	//We really want the gfx to be on the other core.
	xTaskCreatePinnedToCore(&gfxinit_task, "gfx", 16*1024, NULL, 2, NULL, 1);
	xSemaphoreTake(rdy_sem, portMAX_DELAY);
	vSemaphoreDelete(rdy_sem);
}

int old_btns;

int gfx_get_key() {
	const int keys[]={INPUT_LFLIP,INPUT_RFLIP,INPUT_F1,INPUT_SPRING};
	int r=0;
	int new_btns=get_btn_bitmap();
	for (int i=0; i<4; i++) {
		if ((new_btns^old_btns)&(1<<i)) {
			r=keys[i];
			if (old_btns&(1<<i)) r|=INPUT_RELEASE;
		}
	}
	old_btns=new_btns;
	return r;
}



uint64_t last_frame;

uint64_t fps_ts;
int fps_frames;
int skipped;

int current_frame=0;

int gfx_frame_done() {
	return (lcd_get_frame()!=current_frame);
}

#define FRAME_TIME (1000000/60)

void gfx_show(uint8_t *buf, uint32_t *pal, int h, int w, int scroll) {
	uint64_t ts=esp_timer_get_time();
	if (ts-fps_ts > (1000000*5)) {
		printf("%d fps\n", fps_frames/5);
		printf("Free internal: %d\n", heap_caps_get_free_size(MALLOC_CAP_8BIT|MALLOC_CAP_INTERNAL));
		cpu_addr_dump_hitctr();
		fps_frames=0;
		fps_ts=ts;
	}
	fps_frames++;

	lcd_show(buf, pal, h, w, scroll-32);
	current_frame=lcd_get_frame();
	last_frame=ts;
}

