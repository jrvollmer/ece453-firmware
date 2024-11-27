#include <FreeRTOS.h>
#include <math.h>
#include "app_bt_car.h"
#include "dc_motor.h"
#include "task_console.h"
#include "task_color_sensor.h"
#include "task_car.h"

#define IR_RECEIVER_PIN P10_3

#define MOVING_MIN_Y    (0.25)


static volatile bool i_am_hit = false;

// ISR function to detect hits
static void ir_receiver_pin_isr(void *handler_arg, cyhal_gpio_event_t event) {
    i_am_hit = true;
};

// object to register ISR function
cyhal_gpio_callback_data_t ir_receiver_callback_data = {
    .callback = ir_receiver_pin_isr,
    .callback_arg = NULL
};



void task_car_init() {
    // initialize IR Receiver pin as GPIO input
    cy_rslt_t rslt1;
    rslt1 = cyhal_gpio_init(IR_RECEIVER_PIN, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_NONE, false);
    CY_ASSERT(CY_RSLT_SUCCESS == rslt1);
    // register ISR 
    cyhal_gpio_register_callback(IR_RECEIVER_PIN, &ir_receiver_callback_data);
    /* Enable falling edge interrupt event with interrupt priority set to 3 */
    cyhal_gpio_enable_event(IR_RECEIVER_PIN, CYHAL_GPIO_IRQ_FALL, 3, true);


    // create the task
    BaseType_t rslt = xTaskCreate(task_car,
                                  "Car",
                                  configMINIMAL_STACK_SIZE * 3,
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
    bool restore_after_hit = false;
    
    while(1) {
        xQueueReceive(q_color_sensor, &terrain, 10);
        switch(terrain) {
			case WHITE: 
				speed = 100; // TODO Increase speed for a period of time
				break;
            case BROWN_ROAD:
				speed = 75;
				break;
			case GREEN_GRASS:
				speed = 10;
				break;
			case PINK:
                speed = 75;
				// give powerup
                if (prev_terrain != PINK) {
                    task_print("giving powerup via color sensor\n");
                    if (app_bt_car_get_new_item() == pdTRUE) {
                        task_print("successfully got item\n");
                    } else {
                        task_print("error when getting item\n");
                    }
                }
				break;
		}
        prev_terrain = terrain;
        
        if (i_am_hit) {
            turn_dc_motor_off();
            vTaskDelay(5000);
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
                        vTaskDelay(DC_MOTOR_RAMP_STEP_MS);
                    }
                } while (!reached_y);

                prev_y = y;
                restore_after_hit = false;
            }
        }       
    }
}
