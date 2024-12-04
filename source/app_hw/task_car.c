#include <FreeRTOS.h>
#include <task.h>
#include <math.h>
#include "app_bt_car.h"
#include "dc_motor.h"
#include "task_audio.h"
#include "task_console.h"
#include "task_color_sensor.h"
#include "task_car.h"
#include "data/audio_sample_luts.h"

#define IR_RECEIVER_PIN_A P10_3
#define IR_RECEIVER_PIN_B P10_4
#define IR_RECEIVER_PIN_C P10_5

#define RACE_INACTIVE_DELAY_MS    (50)
#define MOVING_MIN_Y    (0.25)

QueueHandle_t q_car;
TimerHandle_t speed_timer;
TimerHandle_t shield_timer;
TimerHandle_t pink_timer;
bool speed_active = false;
bool shield_active = false;
bool can_get_new_powerup = true;

static volatile bool i_am_hit = false;

// ISR function to detect hits
static void ir_receiver_pin_isr(void *handler_arg, cyhal_gpio_event_t event) {
    // Only count hits when racing 
    // AND when shield is inactive
    i_am_hit = ((race_state == RACE_STATE_ACTIVE) && !shield_active);
    if (i_am_hit)
    {
        xTaskNotify(xTaskAudioHandle, (uint32_t)AUDIO_SOUND_EFFECT_HIT, eSetValueWithOverwrite);
    }
};

// object to register ISR function
cyhal_gpio_callback_data_t ir_receiver_callback_data = {
    .callback = ir_receiver_pin_isr,
    .callback_arg = NULL
};

// disable speed boost when timer expires
void speed_timer_callback() {
    speed_active = false;
}
// disable the shield when the timer expires
void shield_timer_callback() {
    shield_active = false;
}
// allow player to get another powerup when timer expires.
void pink_timer_callback() {
    can_get_new_powerup = true;
}

void task_car_init() {
    // initialize queue to receive powerup usage info
    q_car = xQueueCreate(1, sizeof(car_item_t));

    speed_timer = xTimerCreate("speed_timer",            // name
                            pdMS_TO_TICKS(5000),         // expires after 5 secs
                            pdFALSE,                     // one shot timer 
                            ( void * ) 0,                /* The ID is used to store a count of the number of times the timer has expired, which is initialised to 0. */
                            speed_timer_callback         // callback function
                            );

    shield_timer = xTimerCreate("shield_timer",            // name
                            pdMS_TO_TICKS(10000),         // expires after 10 secs
                            pdFALSE,                     // one shot timer 
                            ( void * ) 0,                /* The ID is used to store a count of the number of times the timer has expired, which is initialised to 0. */
                            shield_timer_callback         // callback function
                            );
    pink_timer = xTimerCreate("pink_timer",            // name
                            pdMS_TO_TICKS(5000),         // expires after 10 secs
                            pdFALSE,                     // one shot timer 
                            ( void * ) 0,                /* The ID is used to store a count of the number of times the timer has expired, which is initialised to 0. */
                            pink_timer_callback         // callback function
                            );


    // initialize the THREE (3) IR Receiver pins as GPIO input
    cy_rslt_t rslt1;
    rslt1 = cyhal_gpio_init(IR_RECEIVER_PIN_A, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_NONE, false);
    CY_ASSERT(CY_RSLT_SUCCESS == rslt1);
    // register ISR 
    cyhal_gpio_register_callback(IR_RECEIVER_PIN_A, &ir_receiver_callback_data);
    /* Enable falling edge interrupt event with interrupt priority set to 3 */
    cyhal_gpio_enable_event(IR_RECEIVER_PIN_A, CYHAL_GPIO_IRQ_FALL, 3, true);

    rslt1 = cyhal_gpio_init(IR_RECEIVER_PIN_B, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_NONE, false);
    CY_ASSERT(CY_RSLT_SUCCESS == rslt1);
    cyhal_gpio_register_callback(IR_RECEIVER_PIN_B, &ir_receiver_callback_data);
    cyhal_gpio_enable_event(IR_RECEIVER_PIN_B, CYHAL_GPIO_IRQ_FALL, 3, true);

    // rslt1 = cyhal_gpio_init(IR_RECEIVER_PIN_C, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_NONE, false);
    // CY_ASSERT(CY_RSLT_SUCCESS == rslt1);
    // cyhal_gpio_register_callback(IR_RECEIVER_PIN_C, &ir_receiver_callback_data);
    // cyhal_gpio_enable_event(IR_RECEIVER_PIN_C, CYHAL_GPIO_IRQ_FALL, 3, true);


    // create the task
    BaseType_t rslt = xTaskCreate(task_car,
                                  "Car",
                                  configMINIMAL_STACK_SIZE * 7,
                                  NULL,
                                  configMAX_PRIORITIES - 5,
                                  NULL);
    if (rslt == pdPASS) {
        task_print("CAR task created\n\r");
    }
    else {
        task_print("CAR task NOT created\n\r");
    }
}



void task_car(void *pvParameters) {
    car_joystick_t y = 0;
    car_joystick_t prev_y = y;
    uint8_t dir = STOPPED;
    turn_dc_motor_off();
    uint8_t speed = 0; 
    color_sensor_terrain_t terrain = BROWN_ROAD;
    color_sensor_terrain_t prev_terrain = BROWN_ROAD;
    car_item_t powerup = CAR_ITEM_MIN;
    bool restore_after_hit = false;
    
    while(1) {
        if (race_state == RACE_STATE_ACTIVE) {
            if (pdTRUE == xQueueReceive(q_car, &powerup, 0)) {
                if (powerup == CAR_ITEM_BOOST) {
                    speed = 100; 
                    speed_active = true;
                    xTimerStart(speed_timer, 0); // start timer to make sure speed boost deactivates in 5 secs
                } else if (powerup == CAR_ITEM_SHIELD) {
                    shield_active = true;
                    xTimerStart(shield_timer, 0); // start timer to make sure shield deactivates in 10 secs
                }
            }
            // keep receiving from color sensor so as to not get stale values
            xQueueReceive(q_color_sensor, &terrain, 10);
            // we still want to get power ups even if we are under influence of speed boost
            if (terrain == PINK) {
                // give powerup
                if (can_get_new_powerup && prev_terrain != PINK) {
                    if (app_bt_car_get_new_item() == pdTRUE) {
                        task_print("successfully got item\n");
                    } else {
                        task_print("error when getting item\n");
                    }
                    can_get_new_powerup = false;
                    xTimerStart(pink_timer, 0);
                }
            }
            prev_terrain = terrain;

            // cases where speed is affected.
            if (!speed_active) {
                switch(terrain) {
                    case WHITE:
                        speed = 100; 
                        speed_active = true;
                        xTimerStart(speed_timer, 0); // start timer to make sure speed boost deactivates in 5 secs
                        task_print("starting speed\n");
                        break;
                    case BROWN_ROAD:
                        speed = 75;
                        break;
                    case GREEN_GRASS:
                        speed = 10;
                        break;
                    case PINK:
                        speed = 75;
                        break;
                }
            }
            
            if (i_am_hit) {
                turn_dc_motor_off();
                vTaskDelay(pdMS_TO_TICKS(5000));
                i_am_hit = false;
                restore_after_hit = true;
            } else {
                xQueueReceive(q_ble_car_joystick_y, &y, portMAX_DELAY);
                if (restore_after_hit || (fabs(y - prev_y) > 0.01)) {
                    int ramp_dir_scalar = 1;
                    if (y < prev_y) {
                        // Ramping in negative direction, if needed
                        ramp_dir_scalar = -1;
                    }

                    // Ramp to setpoint
                    car_joystick_t curr_y = prev_y;
                    bool reached_y;
                    do {
                        reached_y = fabs(y - curr_y) < DC_MOTOR_RAMP_STEP_VAL;

                        if (reached_y) {
                            curr_y = y;
                        } else {
                            curr_y += DC_MOTOR_RAMP_STEP_VAL * ramp_dir_scalar;
                        }

                        if (fabs(curr_y) > MOVING_MIN_Y) {
                            if ((curr_y >= 0) && (dir != FORWARD)) {
                                dir = FORWARD;
                                set_dc_motor_direction(dir);
                            } else if ((curr_y < 0) && (dir != REVERSE)) {
                                dir = REVERSE;
                                set_dc_motor_direction(dir);
                            }

                            set_dc_motor_duty_cycle(speed * fabs(curr_y));
                        } else if (dir != STOPPED) {
                            dir = STOPPED;
                            turn_dc_motor_off();
                        }

                        if (!reached_y) {
                            vTaskDelay(pdMS_TO_TICKS(DC_MOTOR_RAMP_STEP_MS));
                        }
                    } while (!reached_y);

                    prev_y = y;
                    restore_after_hit = false;
                }
            }
        } else {
            // Idle while we're waiting for the race to start
            vTaskDelay(pdMS_TO_TICKS(RACE_INACTIVE_DELAY_MS));
        }
    }
}
