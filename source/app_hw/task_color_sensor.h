/*
 * Author: Bowen Quan Team 01
 */

#ifndef __TASK_COLOR_SENSOR_H__
#define __TASK_COLOR_SENSOR_H__

#include <FreeRTOS.h>
#include <queue.h>
#include <stdint.h>
#include "i2c.h"
#include "task_console.h"

#define VEML3328SL_SUBORDINATE_ADDR     0x10
#define COLOR_CONFIG_REG                0x00
#define COLOR_RED_REG                   0x05
#define COLOR_GREEN_REG                 0x06
#define COLOR_BLUE_REG                  0x07
#define COLOR_DEV_ID_REG                0x0C
#define COLOR_TURN_ON_CMD               0x0000
#define THRESHOLD                       0.1
#define CARDBOARD_RB_THRESHOLD          0.4
#define WHITE_THRESHOLD                 0.75

typedef enum {
    BROWN_ROAD = 0,
    WHITE = 1,
    GREEN_GRASS = 2,
    PINK = 3,
    TRANSITION = 4
} color_sensor_terrain_t;

extern QueueHandle_t q_color_sensor;

void task_color_sensor_init(void);


#endif
