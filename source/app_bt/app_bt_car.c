/**
 * @file app_bt_car.c
 * @author James Vollmer (jrvollmer@wisc.edu) - Team 01
 * @brief
 * Source file for car BLE resources and functionality
 *
 * @version 0.1
 * @date 2024-11-16
 *
 * @copyright Copyright (c) 2024
 */
/******************************************************************************
 * Header Files
 ******************************************************************************/
#include "app_bt_car.h"
#include "app_bt_gatt_handler.h"
#include "task_ir_led.h"
#include "task_audio.h"
#include <task.h>
#include <stdlib.h>


/****************************************************************************
 * Typedefs and Defines
 ***************************************************************************/
// Item types, to be sent to the RC controller app
typedef enum
{
    BLE_GREEN_SHELL   = 0,
    BLE_GREEN_SHELL_3 = 1,
    BLE_ITEM_MAX      = 2
} ble_item_e;


/******************************************************************************
 * Global Variables                                                           *
 ******************************************************************************/
// Number of uses per item
static const uint8_t ble_item_use_counts[BLE_ITEM_MAX] = {
    [BLE_GREEN_SHELL]   = 1,
    [BLE_GREEN_SHELL_3] = 3,
};
// BLE item message to internal item types, as defined in app_bt_car.h
static const car_item_t ble_item_to_car_item[BLE_ITEM_MAX] = {
    [BLE_GREEN_SHELL]   = CAR_GREEN_SHELL,
    [BLE_GREEN_SHELL_3] = CAR_GREEN_SHELL,
};

// Queues
QueueHandle_t q_ble_car_item;
QueueHandle_t q_ble_car_joystick_x;
QueueHandle_t q_ble_car_joystick_y;


/****************************************************************************
 * Function Definitions
 ***************************************************************************/
/**
 * @brief  Queue item(s) and send to the RC controller app.
 *         q_ble_car_item must be empty first. Otherwise,
 *         this function will return immediately.
 * 
 * @note   This function cannot be called from within an ISR
 *         due to a call to uxQueueMessagesWaiting
 * 
 * @return BaseType_t
 * pdTRUE if successfully queued new item to be sent to app, pdFALSE otherwise
 */
BaseType_t app_bt_car_get_new_item(void)
{
    BaseType_t ret = pdFALSE;

    if (uxQueueMessagesWaiting(q_ble_car_item) == 0UL)
    {
        // Item queue is empty. Queue new item(s)
        const ble_item_e item = rand() % BLE_ITEM_MAX;

        // Send if client is registered to receive notifications
        app_rc_controller_get_item[0] = (car_item_t)item;
        app_bt_send_message();

        for (int i=0; i < ble_item_use_counts[item]; i++)
        {
            xQueueSend(q_ble_car_item,
                       &ble_item_to_car_item[item],
                       portMAX_DELAY);
        }

        ret = pdTRUE;
    }

    return ret;
}


/**
 * @brief  Use a queued item
 * 
 * @return BaseType_t
 * pdTRUE if successfully used item, pdFALSE otherwise
 */
BaseType_t app_bt_car_use_item(void)
{
    car_item_t item;
    BaseType_t ret;

    // If we have no items to use, don't block
    ret = xQueueReceive(q_ble_car_item, &item, 0);

    if (ret == pdTRUE)
    {
        // Start IR LED and speaker, sending item with notifications
        ret &= xTaskNotify(xTaskIrLedHandle, (uint32_t)item, eSetValueWithOverwrite);
        ret &= xTaskNotify(xTaskAudioHandle, (uint32_t)item, eSetValueWithOverwrite);
    }

    return ret;
}


/**
 * @brief  Send lap message to RC controller
 * 
 * @return void
 */
void app_bt_car_complete_lap(void)
{
    static car_lap_t lap_count = 1;

    // Send if client is registered to receive notifications
    app_rc_controller_lap[0] = lap_count;
    app_bt_send_message();

    lap_count = (lap_count % BLE_CAR_MAX_LAP_COUNT) + 1;
}


void app_bt_car_init(void)
{
    // Create queues for BLE communication
    q_ble_car_item = xQueueCreate(BLE_CAR_ITEM_QUEUE_LEN, sizeof(car_item_t));
    q_ble_car_joystick_x = xQueueCreate(BLE_CAR_JOYSTICK_QUEUE_LEN, sizeof(car_joystick_t));
    q_ble_car_joystick_y = xQueueCreate(BLE_CAR_JOYSTICK_QUEUE_LEN, sizeof(car_joystick_t));
}

/* END OF FILE [] */
