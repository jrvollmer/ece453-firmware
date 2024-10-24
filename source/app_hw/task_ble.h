/**
 * @file task_ble.h
 * @author James Vollmer (jrvollmer@wisc.edu) - Team 01
 * @brief
 * Header file for CLI for controlling BLE interface
 *
 * @version 0.1
 * @date 2024-09-28
 *
 * @copyright Copyright (c) 2024
 */
#ifndef __TASK_BLE_H__
#define __TASK_BLE_H__

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
#include "../FreeRTOSConfig.h"
#include "../FreeRTOS_CLI.h"

/* Project Includes */
#include "task_console.h"
#include "wiced_bt_dev.h"
#include "app_bt_event_handler.h"
#include "app_bt_gatt_handler.h"
#include "app_hw_device.h"
#include "cycfg_gatt_db.h"


// Device macros
#define MAX_DEVS            (3)
#define DEV_NAME_MAX_LEN    (15)


// BLE message/command type
typedef enum
{
    BLE_SCAN    = 0,
    BLE_PAIR    = 1,
    BLE_CONNECT = 2,
    BLE_SEND    = 3
} ble_message_type_t;

// BLE action
typedef enum
{
    BLE_ACTION_PAIR          = 0,
    BLE_ACTION_UNPAIR        = 1,
    BLE_ACTION_CHECK_CONN    = 2,
    BLE_ACTION_NOTIFY        = 3,
    BLE_ACTION_READ_JOYSTICK = 4
} ble_action_t;

// BLE information
typedef struct 
{
    ble_message_type_t msg;
    ble_action_t action;
    uint8_t data;
    QueueHandle_t return_queue;
} ble_packet_t;


extern QueueHandle_t q_ble_req;
extern QueueHandle_t q_ble_resp;
void task_ble_init(void);

#endif // __TASK_BLE_H__
