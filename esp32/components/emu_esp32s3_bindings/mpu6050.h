#pragma once
#include <stdint.h>

typedef struct {
	int16_t accelx;
	int16_t accely;
	int16_t accelz;
	int16_t gyrox;
} mpu6050_accel_tp;

int mpu6050_read_fifo(int i2c_num, mpu6050_accel_tp *meas, int maxct);

void mpu6050_poll(int i2c_num, int *x, int *y, int *z);
int mpu6050_init(int i2c_num);
void mpu6050_sleep(int i2c_num);