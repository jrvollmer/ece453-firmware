#include "task_car.h"

void task_car_init() {
    // create the task
    BaseType_t rslt = xTaskCreate(task_car,
                                  "Car",
                                  configMINIMAL_STACK_SIZE,
                                  NULL,
                                  configMAX_PRIORITIES - 5,
                                  NULL);
    if (rslt == pdPASS) {
        task_print("state machine task created\n\r");
    }
    else {
        task_print("state machine task NOT created\n\r");
    }
}



void task_car(void *pvParameters) {
    car_joystick_t y_dir = 0;
    
    while(1) {
        xQueueReceive(q_ble_car_joystick_y, &y_dir, portMAX_DELAY);

        if (y_dir > 0.5) {
            // go forwards
            set_dc_motor_direction(FORWARD);
            set_dc_motor_duty_cycle(25); 
        } else if (y_dir < -0.5) {
            // go backwards
            set_dc_motor_direction(REVERSE);
            set_dc_motor_duty_cycle(25);
        } else {
            // turn motor off
            turn_dc_motor_off();
        }
    }
}
