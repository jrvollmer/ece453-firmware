/**
 * @file app_ir_led.h
 * @author James Vollmer (jrvollmer@wisc.edu) - Team 01
 * @brief
 * Header file for IR LED interface
 *
 * @version 0.1
 * @date 2024-11-16
 *
 * @copyright Copyright (c) 2024
 */
#ifndef __APP_IR_LED_H__
#define __APP_IR_LED_H__

// FreeRTOS includes
#include <FreeRTOS.h>

// Project includes
#include "app_bt_car.h"


void app_ir_led_init(void);
BaseType_t app_ir_led_use_item(car_item_t item);


#endif // __APP_IR_LED_H__
