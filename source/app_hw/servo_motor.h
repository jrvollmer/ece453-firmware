#ifndef __SERVO_MOTOR_H__
#define __SERVO_MOTOR_H__

#include "cyhal.h"
#include "cyhal_pwm.h"
#include <FreeRTOS.h>
#include "queue.h"
#include "app_bt_car.h"
#include "task_console.h"

#define SERVO_PIN P10_6

#define LEFT45 10
#define LEFT30 8.75
#define TURN_DUTY_RANGE  2.5
#define STRAIGHT 7.5
#define RIGHT30 6.25
#define RIGHT45 5


void task_servo_init();

void set_servo_motor_duty_cycle(float duty_cycle);

#endif