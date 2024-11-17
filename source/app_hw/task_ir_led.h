/**
 * @file task_ir_led.h
 * @author James Vollmer (jrvollmer@wisc.edu) - Team 01
 * @brief
 * Header file for CLI for controlling IR LED
 *
 * @version 0.1
 * @date 2024-11-15
 *
 * @copyright Copyright (c) 2024
 */
#ifndef __TASK_IR_LED_H__
#define __TASK_IR_LED_H__

/* Include Infineon BSP Libraries */
#include "cy_pdl.h"
#include "cyhal.h"
#include "cybsp.h"

/* Include Standard C Libraries*/
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>

/* FreeRTOS Includes */
#include <FreeRTOS.h>
#include <event_groups.h>
#include <queue.h>
#include <semphr.h>
#include <task.h>
#include "source/FreeRTOSConfig.h"
#include "source/FreeRTOS_CLI.h"

/* Include Project Specific Files */
#include "task_console.h"


// IR LED command type
typedef enum
{
    IR_LED_SET_STATE = 0
} ir_led_cmd_type_t;

// Information to pass between CLI handler and IR LED task
typedef struct 
{
    ir_led_cmd_type_t cmd;
    bool state;
    QueueHandle_t return_queue;
} ir_led_packet_t;


extern QueueHandle_t q_ir_led_cli_req;
extern QueueHandle_t q_ir_led_cli_resp;

extern TaskHandle_t xTaskIrLedHandle;


void task_ir_led_init(void);


#endif // __TASK_IR_LED_H__
