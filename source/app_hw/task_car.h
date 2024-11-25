#ifndef __TASK_CAR__
#define __TASK_CAR__

#include <FreeRTOS.h>
#include "app_bt_car.h"
#include "dc_motor.h"
#include "servo_motor.h"
#include "task_console.h"
#include "task_color_sensor.h"
#include "task_ble.h"

// initializes the task
void task_car_init();

void task_car(void *pvParameters);


#endif
