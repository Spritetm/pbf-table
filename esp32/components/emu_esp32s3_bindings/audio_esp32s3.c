//Audio driver. Because the S3 doesn't have a DAC, we use PDM (delta-sigma) output,
//combined with an external analog Sallen-Key lowpass filter. This should actually
//give a better output than the 8-bit DAC in the ESP32.
/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
 * this notice you can do whatever you want with this stuff. If we meet some day, 
 * and you think this stuff is worth it, you can buy me a beer in return. 
 * ----------------------------------------------------------------------------
 */
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include <driver/i2s_pdm.h>
#include "gfx.h"
#include "soc/rtc.h"
#include "soc/rtc_periph.h"
#include "esp_log.h"


//Implement a lock, as we don't want to run the modplayer callback while
//the main emu is sending commands to it as well.
static SemaphoreHandle_t audio_mux;

#define TAG "audio"

static audio_cb_t audio_cb;

void audio_lock() {
	xSemaphoreTake(audio_mux, portMAX_DELAY);
}

void audio_unlock() {
	xSemaphoreGive(audio_mux);
}

static i2s_chan_handle_t tx_handle;

#define SND_CHUNKSZ 512

void audio_task(void *arg) {
	int16_t snd_in[SND_CHUNKSZ]={0};

	while (1) {
		//Get a chunk of audio data from the source...
		xSemaphoreTake(audio_mux, portMAX_DELAY);
		audio_cb(NULL, (uint8_t*)snd_in, sizeof(snd_in));
		xSemaphoreGive(audio_mux);

		//send it
		i2s_channel_write(tx_handle, snd_in, sizeof(snd_in), NULL, portMAX_DELAY);
	}
}


void audio_init(int samprate, audio_cb_t cb) {
	i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
	ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &tx_handle, NULL));
	/* Init the channel into PDM TX mode */
	i2s_pdm_tx_config_t pdm_tx_cfg = {
		.clk_cfg = I2S_PDM_TX_CLK_DEFAULT_CONFIG(samprate),
		.slot_cfg = I2S_PDM_TX_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
		.gpio_cfg = {
			.clk = -1,
			.dout = GPIO_NUM_42,
		},
	};
	ESP_ERROR_CHECK(i2s_channel_init_pdm_tx_mode(tx_handle, &pdm_tx_cfg));
	ESP_ERROR_CHECK(i2s_channel_enable(tx_handle));

	audio_mux=xSemaphoreCreateMutex();
	audio_cb=cb;
	xTaskCreatePinnedToCore(&audio_task, "snd", 32*1024, NULL, 3, NULL, 1);
	ESP_LOGI(TAG, "Audio inited.");
}
