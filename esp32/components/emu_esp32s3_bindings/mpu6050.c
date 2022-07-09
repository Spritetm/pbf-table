#include <stdio.h>
#include "driver/i2c.h"
#include "mpu6050.h"

#define MPU_ADDR 0x68

static esp_err_t setreg(int i2c_num, int reg, int val) {
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, ( (MPU_ADDR<<1) | I2C_MASTER_WRITE), true);
	i2c_master_write_byte(cmd, reg, true);
	i2c_master_write_byte(cmd, val, true);
	i2c_master_stop(cmd);
	esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd, pdMS_TO_TICKS(30));
	if (ret!=ESP_OK) printf("MPU6050: NACK setting reg 0x%X to val 0x%X\n", reg, val);
	i2c_cmd_link_delete(cmd);
	return ret;
}

static int getreg(int i2c_num, int reg) {
	unsigned char byte;
	esp_err_t ret;
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, ( (MPU_ADDR<<1) | I2C_MASTER_WRITE), true);
	i2c_master_write_byte(cmd, reg, true);
	ret=i2c_master_cmd_begin(i2c_num, cmd, pdMS_TO_TICKS(100));
	i2c_cmd_link_delete(cmd);

	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, ( (MPU_ADDR<<1) | I2C_MASTER_READ), true);
	i2c_master_read_byte(cmd, &byte, true);
	i2c_master_stop(cmd);
	ret=i2c_master_cmd_begin(i2c_num, cmd, pdMS_TO_TICKS(100));
	i2c_cmd_link_delete(cmd);
	if (ret!=ESP_OK) printf("MPU6050: NACK reading reg 0x%X\n", reg);
	if (ret!=ESP_OK) {
		return -1;
	}
	return byte;
}



esp_err_t mpu6050_start(int i2c_num, int samp_div) {
	printf("Initializing MPU6050...\n");
	setreg(i2c_num, 107, 0x80);		//Reset
	vTaskDelay(10);
	int i=getreg(i2c_num, 0x75);
	if (i!=0x68) {
		printf("No MPU6050 detected at i2c addr %x! (%d)\n", MPU_ADDR, i);
		return ESP_ERR_NOT_FOUND;
	}
	setreg(i2c_num, 107, 1);		//Take out of sleep, gyro as clk
	setreg(i2c_num, 108, 0);		//Un-standby everything
	setreg(i2c_num, 106, 0x0C);		//reset more stuff
	vTaskDelay(10);
	setreg(i2c_num, 106, 0x0);		//reset more stuff
	setreg(i2c_num, 25, samp_div);	//Sample divider
	setreg(i2c_num, 26, (7<<3)|0);	//fsync to accel_z[0], 260Hz bw (making Fsamp 8KHz)
	setreg(i2c_num, 27, 0);			//gyro def
	setreg(i2c_num, 28, (3<<3));	//accel 8G
	setreg(i2c_num, 35, 0x48);		//fifo: emit accel data plus x gyro
	setreg(i2c_num, 36, 0x00);		//no slave
	setreg(i2c_num, 106, 0x40);		//fifo: enable
	printf("MPU6050 found and initialized.\n");
	
	return ESP_OK;
}


int mpu6050_read_fifo(int i2c_num, mpu6050_accel_tp *meas, int maxct) {
	int i;
	int no;
	uint8_t buf[8];
	i2c_cmd_handle_t cmd;
	esp_err_t ret;
	no=(getreg(i2c_num, 114)<<8);
	no|=getreg(i2c_num, 115);
	if (no>0x8000) {
		printf("Huh? Fifo has %x bytes.\n", no);
		return 0; //huh?
	}
//	printf("FIFO: %d\n", no);
	no=no/8; //bytes -> samples
	if (no>maxct) no=maxct;

	for (i=0; i<no; i++) {
		cmd = i2c_cmd_link_create();
		i2c_master_start(cmd);
		i2c_master_write_byte(cmd, ( (MPU_ADDR<<1) | I2C_MASTER_WRITE), true);
		i2c_master_write_byte(cmd, 116, true);
		i2c_master_stop(cmd);
		ret=i2c_master_cmd_begin(i2c_num, cmd, pdMS_TO_TICKS(100));
		i2c_cmd_link_delete(cmd);

		cmd = i2c_cmd_link_create();
		i2c_master_start(cmd);
		i2c_master_write_byte(cmd, ( (MPU_ADDR<<1) | I2C_MASTER_READ), true);
		i2c_master_read(cmd, buf, 7, 0);
		i2c_master_read(cmd, buf+7, 1, 1);
		i2c_master_stop(cmd);
		ret=i2c_master_cmd_begin(i2c_num, cmd, pdMS_TO_TICKS(100));
		if (ret!=ESP_OK) {
			printf("Error reading packet %d/%d\n", i, no);
			return 0;
		}
		meas[i].accelx=(buf[0]<<8)|buf[1];
		meas[i].accely=(buf[2]<<8)|buf[3];
		meas[i].accelz=(buf[4]<<8)|buf[5];
		meas[i].gyrox=(buf[6]<<8)|buf[7];
		i2c_cmd_link_delete(cmd);
	}
	return no;
}

void mpu6050_sleep(int i2c_num) {
	setreg(i2c_num, 106, 0x02);		//kill fifo
//	setreg(i2c_num, 55, 0xD0);		//enable int, active lvl hi, int active until any read
	setreg(i2c_num, 107, 0x08);		//8MHz clock, temp sens disabled
	setreg(i2c_num, 108, 0x07);		//disable gyro, wake up at 1.25Hz
	setreg(i2c_num, 107, 0x28);		//8MHz clock, cycle mode, temp sens disabled
	setreg(i2c_num, 28, 2);			//accel 8G

#if 0
	while(1) {
		vTaskDelay(10);
		int16_t x=(getreg(i2c_num, 59)<<8)+getreg(i2c_num, 60);
		int16_t y=(getreg(i2c_num, 61)<<8)+getreg(i2c_num, 62);
		int16_t z=(getreg(i2c_num, 63)<<8)+getreg(i2c_num, 64);
		printf("%d %d %d\n", x, y, z);
	}
#endif
}

int mpu6050_init(int i2c_num) {
	//MPU has sample rate of 8KHz. We have an 1K fifo where we put 6byte samples --> about 128 samples.
	//We want to grab the data at about 10Hz -> sample rate of about 1KHz.
	esp_err_t r=mpu6050_start(i2c_num, 64);
	if (r!=ESP_OK) {
		return 0;
	}
	return 1;
}

