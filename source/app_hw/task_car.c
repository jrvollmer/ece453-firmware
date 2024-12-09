#include <FreeRTOS.h>
#include <task.h>
#include <math.h>
#include "app_bt_car.h"
#include "task_ir_led.h"
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
#define IR_RECEIVER_THRESHOLD     (5)

QueueHandle_t q_car;
static TimerHandle_t speed_timer;
static TimerHandle_t shield_timer;
static TimerHandle_t pink_timer;
static TimerHandle_t ir_receiver_timer;
static TimerHandle_t i_frame_timer;
static uint8_t ir_receiver_count[3] = {0,0,0};
static bool speed_active = false;
static bool shield_active = false;
static bool can_get_new_powerup = true;

static volatile bool i_am_hit = false;
static bool prev_i_am_hit = false;


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
// count up if IR receivers are activated   
void ir_receiver_timer_callback() {
    // only check for hits when racing AND we have no shield AND we aren't hit
    if ((race_state == RACE_STATE_ACTIVE) && !shield_active && !i_am_hit) {
        // when IR receiver is activated, the pin is low
        if (!cyhal_gpio_read(IR_RECEIVER_PIN_A)) {
            ir_receiver_count[0]++;
        }
        if (!cyhal_gpio_read(IR_RECEIVER_PIN_B)) {
            ir_receiver_count[1]++;
        }
        if (!cyhal_gpio_read(IR_RECEIVER_PIN_C)) {
            ir_receiver_count[2]++;
        }
        // check if any pins counted a hit
        if (ir_receiver_count[0] > IR_RECEIVER_THRESHOLD || ir_receiver_count[1] > IR_RECEIVER_THRESHOLD || ir_receiver_count[2] > IR_RECEIVER_THRESHOLD) {
            i_am_hit = true;
            // reset all counts
            ir_receiver_count[0] = 0;
            ir_receiver_count[1] = 0;
            ir_receiver_count[2] = 0;
        }
    }
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
    ir_receiver_timer = xTimerCreate("ir_receiver_timer",            // name
                            pdMS_TO_TICKS(10),          // check ir receivers every 10ms
                            pdTRUE,                     // timer repeats
                            ( void * ) 0,               /* The ID is used to store a count of the number of times the timer has expired, which is initialised to 0. */
                            ir_receiver_timer_callback         // callback function
                            );
    i_frame_timer = xTimerCreate("i_frame_timer",            // name
                            pdMS_TO_TICKS(1000),          // give car shield for 1 second so it can't disable itself
                            pdFALSE,                     // one shot timer
                            ( void * ) 0,               /* The ID is used to store a count of the number of times the timer has expired, which is initialised to 0. */
                            shield_timer_callback         // callback function
                            );


    // initialize the THREE (3) IR Receiver pins as GPIO input
    cy_rslt_t rslt1;
    rslt1 = cyhal_gpio_init(IR_RECEIVER_PIN_A, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_NONE, false);
    CY_ASSERT(CY_RSLT_SUCCESS == rslt1);

    rslt1 = cyhal_gpio_init(IR_RECEIVER_PIN_B, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_NONE, false);
    CY_ASSERT(CY_RSLT_SUCCESS == rslt1);

    rslt1 = cyhal_gpio_init(IR_RECEIVER_PIN_C, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_NONE, false);
    CY_ASSERT(CY_RSLT_SUCCESS == rslt1);


    // create the task
    BaseType_t rslt = xTaskCreate(task_car,
                                  "Car",
                                  configMINIMAL_STACK_SIZE * 8,
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
    float32_t prev_scaled_speed = y;
    uint8_t dir = STOPPED;
    turn_dc_motor_off();
    uint8_t speed = 0; 
    color_sensor_terrain_t terrain = BROWN_ROAD;
    color_sensor_terrain_t prev_terrain = BROWN_ROAD;
    car_item_t powerup = CAR_ITEM_MIN;
    bool restore_after_hit = false;
    // start the ir_receiver_timer
    xTimerStart(ir_receiver_timer, 0);
    
    while(1) {
        if (race_state == RACE_STATE_ACTIVE) {
            if (!prev_i_am_hit && i_am_hit) {
                xTaskNotify(xTaskAudioHandle, (uint32_t)AUDIO_SOUND_EFFECT_HIT, eSetValueWithOverwrite);
            }
            prev_i_am_hit = i_am_hit;

            if (pdTRUE == xQueueReceive(q_car, &powerup, 0)) {
                switch (powerup) {
                    case CAR_ITEM_BOOST:
                        speed = 100;
                        speed_active = true;
                        xTimerStart(speed_timer, 0); // start timer to make sure speed boost deactivates in 5 secs
                        break;
                    case CAR_ITEM_SHIELD:
                        shield_active = true;
                        xTimerStart(shield_timer, 0); // start timer to make sure shield deactivates in 10 secs
                        break;
                    case CAR_ITEM_SHOT:
                        // give the car invincibility frames when shooting
                        shield_active = true;
                        xTimerStart(i_frame_timer, 0); // start timer to make sure shield (I-frame) deactivates in 1 sec
                        xTaskNotify(xTaskIrLedHandle, (uint32_t)powerup, eSetValueWithOverwrite);
                        break;
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

            // cases where speed is affected.
            if (!speed_active) {
                switch(terrain) {
                    case WHITE:
                        speed = 100; 
                        speed_active = true;
                        // Start timer to make sure speed boost deactivates in 5 sec
                        xTimerStart(speed_timer, 0);
                        xTaskNotify(xTaskAudioHandle, (uint32_t)AUDIO_SOUND_EFFECT_BOOST, eSetValueWithOverwrite);
                        break;
                    case BROWN_ROAD:
                        speed = 50;
                        break;
                    case GREEN_GRASS:
                        speed = 25;
                        break;
                    case PINK:
                        speed = 50;
                        break;
                    default:
                        break;
                }
            }
            prev_terrain = terrain;

            if (i_am_hit) {
                turn_dc_motor_off();
                vTaskDelay(pdMS_TO_TICKS(5000)); // TODO Replacce with timer like the power ups
                i_am_hit = false;
                restore_after_hit = true;
            } else {
                // Wait for twice the expected write period from the RC controller
                xQueueReceive(q_ble_car_joystick_y, &y, pdMS_TO_TICKS(200));

                float32_t scaled_speed = speed * y;

                if (restore_after_hit || (fabs(scaled_speed - prev_scaled_speed) > DC_MOTOR_RAMP_STEP_VAL)) {
                    int ramp_dir_scalar = 1;
                    if (scaled_speed < prev_scaled_speed) {
                        // Ramping in negative direction, if needed
                        ramp_dir_scalar = -1;
                    }

                    // Ramp to setpoint
                    float32_t curr_scaled_speed = prev_scaled_speed;
                    bool reached_target;
                    do {
                        reached_target = fabs(scaled_speed - curr_scaled_speed) < DC_MOTOR_RAMP_STEP_VAL;

                        if (reached_target) {
                            curr_scaled_speed = scaled_speed;
                        } else {
                            curr_scaled_speed += DC_MOTOR_RAMP_STEP_VAL * ramp_dir_scalar;
                        }

                        if (fabs(curr_scaled_speed) > DC_MOTOR_MIN_DUTY) {
                            if ((curr_scaled_speed >= 0) && (dir != FORWARD)) {
                                dir = FORWARD;
                                set_dc_motor_direction(dir);
                            } else if ((curr_scaled_speed < 0) && (dir != REVERSE)) {
                                dir = REVERSE;
                                set_dc_motor_direction(dir);
                            }

                            set_dc_motor_duty_cycle(fabs(curr_scaled_speed));
                        } else if (dir != STOPPED) {
                            dir = STOPPED;
                            turn_dc_motor_off();
                        }

                        if (!reached_target) {
                            vTaskDelay(pdMS_TO_TICKS(DC_MOTOR_RAMP_STEP_MS));
                        }
                    } while (!reached_target);

                    prev_scaled_speed = scaled_speed;
                    restore_after_hit = false;
                }
            }
        } else {
            // Idle while we're waiting for the race to start
            if (dir != STOPPED) {
                dir = STOPPED;
                turn_dc_motor_off();
            }
            vTaskDelay(pdMS_TO_TICKS(RACE_INACTIVE_DELAY_MS));
        }
    }
}
