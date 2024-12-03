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
#include "app_bt_event_handler.h"
#include "task_ir_led.h"
#include "task_audio.h"
#include "data/audio_sample_luts.h"
#include "task_car.h"
#include <task.h>
#include <stdlib.h>


/****************************************************************************
 * Typedefs and Defines
 ***************************************************************************/
// Item types, to be sent to the RC controller app
typedef enum
{
    BLE_ITEM_SHOT   = 0,
    BLE_ITEM_SHOT_3 = 1,
    BLE_ITEM_SHIELD = 2,
    BLE_ITEM_BOOST  = 3,
    BLE_ITEM_MAX    = 4
} ble_item_e;


/******************************************************************************
 * Global Variables                                                           *
 ******************************************************************************/
// Queues
QueueHandle_t q_ble_car_joystick_x;
QueueHandle_t q_ble_car_joystick_y;
// Race state
volatile race_state_e race_state;


/****************************************************************************
 * Function Definitions
 ***************************************************************************/
/**
 * @brief  Send new item to the RC controller app. If we already
 *         have unused items, this will be ignored by the app.
 * 
 * @return BaseType_t
 * pdTRUE if successfully sent new item to app, pdFALSE otherwise
 */
BaseType_t app_bt_car_get_new_item(void)
{
    BaseType_t ret = pdFALSE;

    if (race_state == RACE_STATE_INACTIVE)
    {
        // Not in race yet
        ret = pdTRUE;
    }
    else if (ble_state.conn_id && (race_state == RACE_STATE_ACTIVE))
    {
        // Connected to app
        const ble_item_e item = rand() % BLE_ITEM_MAX;

        // Send if client is registered to receive notifications
        app_rc_controller_get_item[0] = (car_item_t)item;
        app_bt_send_message(HDLC_RC_CONTROLLER_GET_ITEM_VALUE);

        ret = xTaskNotify(xTaskAudioHandle, (uint32_t)AUDIO_SOUND_EFFECT_GET_ITEM, eSetValueWithOverwrite);
    }

    return ret;
}


/**
 * @brief  Use an item
 * 
 * @param car_item_t
 * The individual item to use
 * @return BaseType_t
 * pdTRUE if successfully used item, pdFALSE otherwise
 */
BaseType_t app_bt_car_use_item(car_item_t item)
{
    BaseType_t ret = pdFALSE;

    if (race_state == RACE_STATE_INACTIVE)
    {
        // Not in race yet
        ret = pdTRUE;
    }
    else if ((race_state == RACE_STATE_ACTIVE) && (item > CAR_ITEM_MIN) && (item < CAR_ITEM_MAX))
    {
        // Start speaker and possibly IR LED, sending item/sound effect with notifications
        switch (item)
        {
            case CAR_ITEM_SHOT:
                ret = xTaskNotify(xTaskIrLedHandle, (uint32_t)item, eSetValueWithOverwrite);
                ret &= xTaskNotify(xTaskAudioHandle, (uint32_t)AUDIO_SOUND_EFFECT_USE_SHOT, eSetValueWithOverwrite);
                break;
            case CAR_ITEM_SHIELD:
                // TODO Protect against hits
                xQueueSend(q_car, &item, 0);
                ret = xTaskNotify(xTaskAudioHandle, (uint32_t)AUDIO_SOUND_EFFECT_USE_SHIELD, eSetValueWithOverwrite);
                break;
            case CAR_ITEM_BOOST:
                // TODO Increase speed
                xQueueSend(q_car, &item, 0);
                ret = xTaskNotify(xTaskAudioHandle, (uint32_t)AUDIO_SOUND_EFFECT_BOOST, eSetValueWithOverwrite);
                break;
            default:
                break;
        }
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
    static car_lap_t lap_count = 0;

    if (race_state == RACE_STATE_ACTIVE)
    {
        // Lap count will be set/reset by app on race state changes
        const car_lap_t rc_controller_lap = app_rc_controller_lap[0];
        if (((rc_controller_lap == 0) && (lap_count != 0)) || (rc_controller_lap > lap_count))
        {
            lap_count = rc_controller_lap;
        }

        // Don't need to send actual lap count to app, as long as it's greater than 1
        // App will keep its own lap count
        lap_count++;

        // Send if client is registered to receive notifications
        app_rc_controller_lap[0] = lap_count;
        app_bt_send_message(HDLC_RC_CONTROLLER_LAP_VALUE);
    }
}


void app_bt_car_init(void)
{
    // Create queues for BLE communication
    q_ble_car_joystick_x = xQueueCreate(BLE_CAR_JOYSTICK_QUEUE_LEN, sizeof(car_joystick_t));
    q_ble_car_joystick_y = xQueueCreate(BLE_CAR_JOYSTICK_QUEUE_LEN, sizeof(car_joystick_t));

    // Initial race state
    race_state = RACE_STATE_INACTIVE;
}

/* END OF FILE [] */
