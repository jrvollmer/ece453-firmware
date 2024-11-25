#include <FreeRTOS.h>
#include "app_bt_car.h"
#include "dc_motor.h"
#include "task_console.h"
#include "task_color_sensor.h"
#include "task_car.h"

#define IR_RECEIVER_PIN P10_2

volatile bool i_am_hit = false;

// ISR function to detect hits
void ir_receiver_pin_isr(void *handler_arg, cyhal_gpio_event_t event) {
    i_am_hit = true;
}

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
    car_joystick_t y_dir = 0;
    turn_dc_motor_off();
    uint8_t speed = 0; 
    color_sensor_terrain_t terrain = BROWN_ROAD;
    bool powerup_given = false;
    
    while(1) {
        xQueueReceive(q_color_sensor, &terrain, 10);
        switch(terrain) {
			case WHITE: 
				speed = 50;
				break;
            case BROWN_ROAD:
				speed = 30;
				break;
			case GREEN_GRASS:
				speed = 10;
				break;
			case PINK:
                speed = 30;
				// give powerup
                if (!powerup_given) {
                    task_print("giving powerup via color sensor\n");
                    if (app_bt_car_get_new_item() == pdTRUE) {
                        task_print("successfully got item\n");
                    } else {
                        task_print("error when getting item\n");
                    }
                    powerup_given = true;
                }
				break;
		}
        xQueueReceive(q_ble_car_joystick_y, &y_dir, portMAX_DELAY);
        if (i_am_hit) {
            turn_dc_motor_off();
            vTaskDelay(5000);
            i_am_hit = false;
        } else {
            if (y_dir > 0.5) {
                // go forwards
                set_dc_motor_direction(FORWARD);
                set_dc_motor_duty_cycle(speed); 
            } else if (y_dir < -0.5) {
                // go backwards
                set_dc_motor_direction(REVERSE);
                set_dc_motor_duty_cycle(speed);
            } else {
                // turn motor off
                turn_dc_motor_off();
            }
        }       
    }
}
