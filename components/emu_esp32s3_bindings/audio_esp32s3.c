#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include <driver/i2s.h>
#include "gfx.h"
#include "soc/rtc.h"
#include "soc/rtc_periph.h"
#include "esp_log.h"
static SemaphoreHandle_t audio_mux;

#define TAG "audio"

//typedef void(*audio_cb_t)(void* userdata, uint8_t* stream, int len);
audio_cb_t audio_cb;


void audio_lock() {
	xSemaphoreTake(audio_mux, portMAX_DELAY);
}

void audio_unlock() {
	xSemaphoreGive(audio_mux);
}

#define SND_CHUNKSZ 1024

//Something is wrong here, as the audio plays way too fast.
void audio_task(void *arg) {
	int16_t snd_in[SND_CHUNKSZ]={0};
	int32_t snd_out[SND_CHUNKSZ]={0};
	while (1) {
		xSemaphoreTake(audio_mux, portMAX_DELAY);
		audio_cb(NULL, (uint8_t*)snd_in, sizeof(snd_in));
		xSemaphoreGive(audio_mux);
		for (int i=0; i<SND_CHUNKSZ; i++) {
			int v=snd_in[i];
//			v=v/8; //HACK! Volume
			snd_out[i]=(v<<16)|(v);
		}
		size_t written;
		i2s_write(0, snd_out, sizeof(snd_out), &written, portMAX_DELAY);
	}
}


void audio_init(int samprate, audio_cb_t cb) {

	i2s_config_t i2s_config = {
		.mode = I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_PDM,
		.sample_rate = samprate/2,
		.bits_per_sample = 16,
		.communication_format = I2S_COMM_FORMAT_STAND_I2S,
		.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
		.intr_alloc_flags = 0,
		.dma_desc_num = 2,
		.dma_frame_num = 1024,
		.use_apll = 1,
	};
	const i2s_pin_config_t pin_config = {
		.data_out_num=42,
		.mck_io_num= -1,
		.bck_io_num = -1,
		.ws_io_num = -1,
		.data_in_num = -1
	};
	//install and start i2s driver
	i2s_driver_install(0, &i2s_config, 0, NULL);
	i2s_set_pin(0, &pin_config);

	audio_mux=xSemaphoreCreateMutex();
	audio_cb=cb;
	xTaskCreatePinnedToCore(&audio_task, "snd", 32*1024, NULL, 3, NULL, 1);
	ESP_LOGI(TAG, "Audio inited.");
}
