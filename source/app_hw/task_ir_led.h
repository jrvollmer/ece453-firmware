/**
 * @file task_ir_led.h
 * @author James Vollmer (jrvollmer@wisc.edu) - Team 01
 * @brief
 * Header file for CLI for controlling IR LED
 *
 * @version 0.1
 * @date 2024-10-16
 *
 * @copyright Copyright (c) 2024
 */
#ifndef __TASK_IR_LED_H__
#define __TASK_IR_LED_H__

#include "main.h"


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

extern QueueHandle_t q_ir_led_req;
extern QueueHandle_t q_ir_led_resp;

void task_ir_led_init(void);


#endif // __TASK_IR_LED_H__
