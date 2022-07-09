//Secondary SPI backboard LCD driver
/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
 * this notice you can do whatever you want with this stuff. If we meet some day, 
 * and you think this stuff is worth it, you can buy me a beer in return. 
 * ----------------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"

#define PIN_NUM_CS	 7
#define PIN_NUM_DC	 15

#define PARALLEL_LINES 8

/*
 The LCD needs a bunch of command/argument values to be initialized. They are stored in this struct.
*/
typedef struct {
	uint8_t cmd;
	uint8_t data[16];
	uint8_t databytes; //No of data in data; bit 7 = delay after set; 0xFF = end of cmds.
} lcd_init_cmd_t;

const lcd_init_cmd_t init_cmds[17]={
	{0xC8, {0xFF, 0x93, 0x42}, 3},
	{0xb6, {0x0a, 0xE0}, 2},
	{0x36, {0x08}, 1},
	{0x3A, {0x55}, 1},
	{0xC0, {0x13, 0x0E}, 2},
	{0xC1, {0x02}, 1},
	{0xC5, {0xCA}, 1},
	{0xB1, {0x00, 0x1B}, 2},
	{0xB4, {0x02}, 1},
	{0xE0, {0x00, 0x02, 0x04, 0x00, 0x11, 0x08, 0x35, 0x79, 0x45, 0x07, 0x0D, 0x09, 0x16, 0x17, 0x0F}, 15},
	{0xE1, {0x00, 0x28, 0x29, 0x02, 0x0F, 0x06, 0x3F, 0x25, 0x55, 0x06, 0x15, 0x0F, 0x38, 0x38, 0x0F}, 15},
	{0x2A, {0x00, 0x00, 0x01, 0x3F}, 4},
	{0x2B, {0x00, 0x00, 0x00, 0xEF}, 4},
	{0x11, {0}, 0x80},
	{0x29, {0}, 0},
	{0x2C, {0}, 0},
};



/* Send a command to the LCD. Uses spi_device_polling_transmit, which waits
 * until the transfer is complete.
 */
static void lcd_cmd(spi_device_handle_t spi, const uint8_t cmd)
{
	esp_err_t ret;
	spi_transaction_t t;
	memset(&t, 0, sizeof(t));		//Zero out the transaction
	t.length=8;						//Command is 8 bits
	t.tx_buffer=&cmd;				//The data is the cmd itself
	t.user=(void*)0;				//D/C needs to be set to 0
	ret=spi_device_polling_transmit(spi, &t);  //Transmit!
	assert(ret==ESP_OK);			//Should have had no issues.
}

/* Send data to the LCD. Uses spi_device_polling_transmit, which waits until the
 * transfer is complete.
 */
static void lcd_data(spi_device_handle_t spi, const uint8_t *data, int len) {
	esp_err_t ret;
	spi_transaction_t t;
	if (len==0) return;				//no need to send anything
	memset(&t, 0, sizeof(t));		//Zero out the transaction
	t.length=len*8;					//Len is in bytes, transaction length is in bits.
	t.tx_buffer=data;				//Data
	t.user=(void*)1;				//D/C needs to be set to 1
	ret=spi_device_polling_transmit(spi, &t);  //Transmit!
	assert(ret==ESP_OK);			//Should have had no issues.
}

//This function is called (in irq context!) just before a transmission starts. It will
//set the D/C line to the value indicated in the user field.
void lcd_spi_pre_transfer_callback(spi_transaction_t *t) {
	int dc=(int)t->user;
	gpio_set_level(PIN_NUM_DC, dc);
}

//Initialize the display
static void lcd_init_controller(spi_device_handle_t spi) {
	int cmd=0;
	const lcd_init_cmd_t* lcd_init_cmds=init_cmds;

	//Initialize non-SPI GPIOs
	gpio_config_t cfg = {
		.pin_bit_mask = BIT64(PIN_NUM_DC),
		.mode = GPIO_MODE_OUTPUT,
		.pull_up_en = false,
		.pull_down_en = false,
		.intr_type = GPIO_INTR_DISABLE,
	};
	gpio_config(&cfg);

	//Send all the commands
	while (lcd_init_cmds[cmd].databytes!=0xff) {
		lcd_cmd(spi, lcd_init_cmds[cmd].cmd);
		uint8_t d[16];
		memcpy(d, lcd_init_cmds[cmd].data, 16);
		lcd_data(spi, d, lcd_init_cmds[cmd].databytes&0x1F);
		if (lcd_init_cmds[cmd].databytes&0x80) {
			vTaskDelay(pdMS_TO_TICKS(100));
		}
		cmd++;
	}
}


static void send_lines(spi_device_handle_t spi, int ypos, uint16_t *linedata) {
	esp_err_t ret;
	int x;
	//Transaction descriptors. Declared static so they're not allocated on the stack; we need this memory even when this
	//function is finished because the SPI driver needs access to it even while we're already calculating the next line.
	static spi_transaction_t trans[6];

	//In theory, it's better to initialize trans and data only once and hang on to the initialized
	//variables. We allocate them on the stack, so we need to re-init them each call.
	for (x=0; x<6; x++) {
		memset(&trans[x], 0, sizeof(spi_transaction_t));
		if ((x&1)==0) {
			//Even transfers are commands
			trans[x].length=8;
			trans[x].user=(void*)0;
		} else {
			//Odd transfers are data
			trans[x].length=8*4;
			trans[x].user=(void*)1;
		}
		trans[x].flags=SPI_TRANS_USE_TXDATA;
	}
	trans[0].tx_data[0]=0x2A;			//Column Address Set
	trans[1].tx_data[0]=0;				//Start Col High
	trans[1].tx_data[1]=0;				//Start Col Low
	trans[1].tx_data[2]=(319)>>8;		//End Col High
	trans[1].tx_data[3]=(319)&0xff;		//End Col Low
	trans[2].tx_data[0]=0x2B;			//Page address set
	trans[3].tx_data[0]=ypos>>8;		//Start page high
	trans[3].tx_data[1]=ypos&0xff;		//start page low
	trans[3].tx_data[2]=(ypos+PARALLEL_LINES)>>8;	 //end page high
	trans[3].tx_data[3]=(ypos+PARALLEL_LINES)&0xff;	 //end page low
	trans[4].tx_data[0]=0x2C;			//memory write
	trans[5].tx_buffer=linedata;		//finally send the line data
	trans[5].length=320*2*8*PARALLEL_LINES;			 //Data length, in bits
	trans[5].flags=0; //undo SPI_TRANS_USE_TXDATA flag

	//Queue all transactions.
	for (x=0; x<6; x++) {
		ret=spi_device_queue_trans(spi, &trans[x], portMAX_DELAY);
		assert(ret==ESP_OK);
	}

	//When we are here, the SPI driver is busy (in the background) getting the transactions sent. That happens
	//mostly using DMA, so the CPU doesn't have much to do here. We're not going to wait for the transaction to
	//finish because we may as well spend the time calculating the next line. When that is done, we can call
	//send_line_finish, which will wait for the transfers to be done and check their status.
}

static void send_line_finish(spi_device_handle_t spi) {
	spi_transaction_t *rtrans;
	esp_err_t ret;
	//Wait for all 6 transactions to be done and get back the results.
	for (int x=0; x<6; x++) {
		ret=spi_device_get_trans_result(spi, &rtrans, portMAX_DELAY);
		assert(ret==ESP_OK);
		//We could inspect rtrans now if we received any info back. The LCD is treated as write-only, though.
	}
}

#define EVT_TYPE_BBIMG 0
#define EVT_TYPE_DMD 1
typedef struct {
	int evt_type;
	int img;
	uint8_t *vptr;
	uint32_t *pal;
} lcd_evt_t;

static QueueHandle_t lcd_evtq;


typedef struct {
	const uint8_t *data;
	int height;
} images_t;

extern const uint8_t img_loading_rgb_start[] asm("_binary_loading_rgb_start");
extern const uint8_t img_pf_rgb_start[] asm("_binary_pf_rgb_start");
extern const uint8_t img_table1_rgb_start[] asm("_binary_table1_rgb_start");
extern const uint8_t img_table2_rgb_start[] asm("_binary_table2_rgb_start");
extern const uint8_t img_table3_rgb_start[] asm("_binary_table3_rgb_start");
extern const uint8_t img_table4_rgb_start[] asm("_binary_table4_rgb_start");
extern const uint8_t font[] asm("_binary_font_bin_start");

static const images_t images[]={
	{&img_pf_rgb_start[0], 320},
	{&img_loading_rgb_start[0], 320},
	{&img_table1_rgb_start[0], 160},
	{&img_table2_rgb_start[0], 160},
	{&img_table3_rgb_start[0], 160},
	{&img_table4_rgb_start[0], 160}, //don't ask
};

uint8_t fade(int a, int b, int fade) {
	return (a*(255-fade)+b*fade)/256;
}

void fade_bbimg(spi_device_handle_t spi, int cur_img, int next_img, uint16_t *line_a, uint16_t *line_b) {
	uint16_t *line=line_a;
	for (int f=0; f<256+16; f+=16) {
		if (f>256) f=256;
		const uint8_t *imga=images[cur_img].data;
		const uint8_t *imgb=images[next_img].data;
		int maxh=images[cur_img].height;
		if (maxh<images[next_img].height) maxh=images[next_img].height;
		for (int yy=0; yy<240; yy+=PARALLEL_LINES) {
			uint16_t *p=line;
			for (int y=0; y<PARALLEL_LINES; y++) {
				if (y+yy<maxh) {
					for (int x=0; x<320; x++) {
						int r=fade(y+yy<images[cur_img].height?*imga++:0, y+yy<images[next_img].height?*imgb++:0,  f);
						int g=fade(y+yy<images[cur_img].height?*imga++:0, y+yy<images[next_img].height?*imgb++:0,  f);
						int b=fade(y+yy<images[cur_img].height?*imga++:0, y+yy<images[next_img].height?*imgb++:0,  f);
						uint16_t pp=((r>>3)<<11)|((g>>2)<<5)|((b>>3));
						*p++=(pp>>8)|(pp<<8);
					}
				} else {
					for (int x=0; x<320; x++) *p++=0;
				}
			}
			send_line_finish(spi);
			send_lines(spi, yy, line);
			if (line==line_a) line=line_b; else line=line_a; //flip
		}
		vTaskDelay(pdMS_TO_TICKS(10));
	}
}

static void put_dmd(spi_device_handle_t spi, uint8_t *bf, uint32_t *pal, uint16_t *line_a, uint16_t *line_b) {
	uint16_t *line=line_a;

	for (int yy=0; yy<32; yy+=PARALLEL_LINES) {
		uint16_t *p=line;
		for (int y=0; y<PARALLEL_LINES; y++) {
			uint8_t *pl=&bf[336*(yy+y)];
			for (int x=0; x<320; x++) {
				int r,g,b;
				uint16_t pp;
				r=(pal[pl[0]]>>16)&0xff;
				g=(pal[pl[0]]>>8)&0xff;
				b=(pal[pl[0]]>>0)&0xff;
				pp=((r>>3)<<11)|((g>>2)<<5)|((b>>3));
				*p++=(pp>>8)|(pp<<8);
				pl++;
			}
		}
		send_line_finish(spi);
		send_lines(spi, yy+200, line);
		if (line==line_a) line=line_b; else line=line_a; //flip
	}
}

void lcdbb_task(void *arg) {
	spi_device_handle_t spi=(spi_device_handle_t)arg;
	uint16_t *line_a, *line_b, *line;
	line_a=calloc(PARALLEL_LINES, 320*2);
	line_b=calloc(PARALLEL_LINES, 320*2);
	assert(line_a);
	assert(line_b);
	line=line_a;

	//clear entire screen
	send_lines(spi, 0, line);
	for (int y=PARALLEL_LINES; y<240; y+=PARALLEL_LINES) {
		send_line_finish(spi);
		send_lines(spi, y, line);
	}
	int cur_img=0;

	while(1) {
		lcd_evt_t evt;
		xQueueReceive(lcd_evtq, &evt, portMAX_DELAY);
		if (evt.evt_type==EVT_TYPE_BBIMG) {
			fade_bbimg(spi, cur_img, evt.img, line_a, line_b);
			cur_img=evt.img;
		} else if (evt.evt_type==EVT_TYPE_DMD) {
			put_dmd(spi, evt.vptr, evt.pal, line_a, line_b);
		}
	}
}



void backboard_show(int img) {
	lcd_evt_t evt={
		.evt_type=EVT_TYPE_BBIMG,
		.img=img,
	};
	xQueueSend(lcd_evtq, &evt, portMAX_DELAY);
}

void lcdbb_handle_dmd(uint8_t *dmdfb, uint32_t *pal) {
	lcd_evt_t evt={
		.evt_type=EVT_TYPE_DMD,
		.vptr=dmdfb,
		.pal=pal,
	};
	xQueueSend(lcd_evtq, &evt, 0);
}

spi_device_handle_t lcdbb_add_to_bus(int spi_bus) {
	esp_err_t ret;
	spi_device_handle_t spi;
	spi_device_interface_config_t devcfg={
		.clock_speed_hz=20*1000*1000,			//Clock out at 26 MHz
		.mode=0,								//SPI mode 0
		.spics_io_num=PIN_NUM_CS,				//CS pin
		.queue_size=7,							//We want to be able to queue 7 transactions at a time
		.pre_cb=lcd_spi_pre_transfer_callback,	//Specify pre-transfer callback to handle D/C line
	};
	//Attach the LCD to the SPI bus
	ret=spi_bus_add_device(spi_bus, &devcfg, &spi);
	ESP_ERROR_CHECK(ret);
	return spi;
}

void lcdbb_init(spi_device_handle_t spi) {
	//Initialize the LCD
	lcd_init_controller(spi);

	lcd_evtq=xQueueCreate(2, sizeof(lcd_evt_t));
	//Start lcd sending thread
	xTaskCreate(lcdbb_task, "lcdbb", 2*1024, (void*)spi, 5, NULL);
}


