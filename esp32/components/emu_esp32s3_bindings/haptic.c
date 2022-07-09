#include <stdio.h>
#include "driver/gpio.h"
#include "driver/mcpwm.h"
#include "esp_timer.h"

#define HAPTIC_EVT_LFLIPPER 0
#define HAPTIC_EVT_RFLIPPER 1
#define HAPTIC_EVT_BALL 2

#define GPIO_FIN_H 41
#define GPIO_RIN_H 40
#define GPIO_FIN_V 39
#define GPIO_RIN_V 38

//duty cycle [-100..100]
static void set_duty(int unit, int duty_cycle) {
//	printf("Haptic: duty %d\n", duty_cycle);
	if (duty_cycle > 0) {
		mcpwm_set_signal_low(unit, MCPWM_TIMER_0, MCPWM_OPR_A);
		mcpwm_set_duty(unit, MCPWM_TIMER_0, MCPWM_OPR_B, duty_cycle);
		mcpwm_set_duty_type(unit, MCPWM_TIMER_0, MCPWM_OPR_B, MCPWM_DUTY_MODE_0);	 //call this each time, if operator was previously in low/high state
	} else {
		mcpwm_set_signal_low(unit, MCPWM_TIMER_0, MCPWM_OPR_B);
		mcpwm_set_duty(unit, MCPWM_TIMER_0, MCPWM_OPR_A, -duty_cycle);
		mcpwm_set_duty_type(unit, MCPWM_TIMER_0, MCPWM_OPR_A, MCPWM_DUTY_MODE_0); //call this each time, if operator was previously in low/high state
	}
}


static esp_timer_handle_t end_timer;

static int cur_ax=0, cur_ay=0;

static void haptic_end(void *arg) {
	cur_ay/=2;
	cur_ax/=2;
	set_duty(MCPWM_UNIT_0, cur_ay);
	set_duty(MCPWM_UNIT_1, cur_ax);
}

void haptic_event(int event_type, int accel_x, int accel_y) {
	cur_ax=accel_x;
	cur_ay=accel_y;
	set_duty(MCPWM_UNIT_0, cur_ay);
	set_duty(MCPWM_UNIT_1, cur_ax);
//	ESP_ERROR_CHECK(esp_timer_start_once(end_timer, 10*1000));
}

void haptic_init() {
	mcpwm_config_t pwm_config={
		.frequency = 30*1000,
		.cmpr_a = 0,
		.cmpr_b = 0,
		.duty_mode = MCPWM_DUTY_MODE_0,
		.counter_mode = MCPWM_UP_COUNTER,
	};
	ESP_ERROR_CHECK(mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, GPIO_FIN_H));
	ESP_ERROR_CHECK(mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0B, GPIO_RIN_H));
	ESP_ERROR_CHECK(mcpwm_gpio_init(MCPWM_UNIT_1, MCPWM0A, GPIO_FIN_V));
	ESP_ERROR_CHECK(mcpwm_gpio_init(MCPWM_UNIT_1, MCPWM0B, GPIO_RIN_V));
	ESP_ERROR_CHECK(mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config));
	ESP_ERROR_CHECK(mcpwm_init(MCPWM_UNIT_1, MCPWM_TIMER_1, &pwm_config));
	ESP_ERROR_CHECK(mcpwm_start(MCPWM_UNIT_0, MCPWM_TIMER_0));
	ESP_ERROR_CHECK(mcpwm_start(MCPWM_UNIT_1, MCPWM_TIMER_1));
	const esp_timer_create_args_t timer_cfg={
		.callback=haptic_end,
		.name="haptic"
	};
	ESP_ERROR_CHECK(esp_timer_create(&timer_cfg, &end_timer));
	ESP_ERROR_CHECK(esp_timer_start_periodic(end_timer, 10*1000));
}

