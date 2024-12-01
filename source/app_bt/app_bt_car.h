/**
 * @file app_bt_car.h
 * @author James Vollmer (jrvollmer@wisc.edu) - Team 01
 * @brief
 * Header file for car BLE resources and functionality
 *
 * @version 0.1
 * @date 2024-11-16
 *
 * @copyright Copyright (c) 2024
 */
#ifndef __APP_BT_CAR_H__
#define __APP_BT_CAR_H__

/******************************************************************************
 * Header Files
 ******************************************************************************/
// FreeRTOS Includes
#include <FreeRTOS.h>
#include <queue.h>


/****************************************************************************
 * Typedefs and Defines
 ***************************************************************************/
// Queue constants
#define BLE_CAR_JOYSTICK_QUEUE_LEN    (3)

#define BLE_CAR_MAX_LAP_COUNT    (3)

typedef uint8_t car_item_t;
typedef uint8_t car_lap_t;
typedef uint8_t car_event_t;
typedef float32_t car_joystick_t;

// Item types, for internal use
typedef enum
{
    CAR_ITEM_MIN    = 0,
    CAR_ITEM_SHOT   = 1,
    CAR_ITEM_SHIELD = 2,
    CAR_ITEM_BOOST  = 3,
    CAR_ITEM_MAX    = 4
} car_item_e;

typedef enum
{
    CAR_EVENT_RACE_START  = 0,
    CAR_EVENT_RACE_RESUME = 1,
    CAR_EVENT_RACE_END    = 2,
} car_event_e;

typedef enum
{
    RACE_STATE_ACTIVE = 0,
    RACE_STATE_INACTIVE = 1,
    RACE_STATE_MAX = 2,
} race_state_e;

/****************************************************************************
 * Extern Data Declarations
 ***************************************************************************/
// Queues for incoming BLE messages
extern QueueHandle_t q_ble_car_joystick_x;
extern QueueHandle_t q_ble_car_joystick_y;
// Race state
extern volatile race_state_e race_state;


/****************************************************************************
 * Function Declarations
 ***************************************************************************/

void app_bt_car_init(void);
BaseType_t app_bt_car_get_new_item(void);
BaseType_t app_bt_car_use_item(car_item_t item);
void app_bt_car_complete_lap(void);


#endif      /*__APP_BT_CAR_H__ */

/* END OF FILE [] */
