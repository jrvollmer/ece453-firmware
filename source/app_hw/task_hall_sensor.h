/*
 * Author: Bowen Quan Team 01
 */

#ifndef __TASK_HALL_SENSOR_H__
#define __TASK_HALL_SENSOR_H__

#include <FreeRTOS.h>
#include <queue.h>
#include "cyhal_adc.h"
#include "task_console.h"
#include "app_bt_car.h"

#define HALL_SENSOR_ADC_PIN P10_2
// NOTE: 620 = 1V
#define HALL_THRESHOLD 40


void task_hall_sensor_init(void);

#endif