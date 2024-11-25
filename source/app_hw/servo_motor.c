#include "servo_motor.h"

cyhal_pwm_t servo_pwm_obj;


void set_servo_motor_duty_cycle(float duty_cycle) {
    cy_rslt_t rslt;
    rslt = cyhal_pwm_set_duty_cycle(&servo_pwm_obj, duty_cycle, 50);
    rslt = cyhal_pwm_start(&servo_pwm_obj);
    CY_ASSERT(CY_RSLT_SUCCESS == rslt);
}

void task_servo() {
    car_joystick_t x_dir = 0;
    while (1) {
        xQueueReceive(q_ble_car_joystick_x, &x_dir, portMAX_DELAY);
        if (x_dir > 0.5) {
            // remember that the servo is upside down, so left and right are different. 
            set_servo_motor_duty_cycle(LEFT30);
        } else if (x_dir < -0.5) {
            set_servo_motor_duty_cycle(RIGHT30);
        } else {
            set_servo_motor_duty_cycle(STRAIGHT);
        }
        vTaskDelay(10);
    }
}

void task_servo_init() {
    cy_rslt_t rslt1;
    
    /* Initialize PWM on the supplied pin and assign a new clock */
    rslt1 = cyhal_pwm_init(&servo_pwm_obj, SERVO_PIN, NULL);
    CY_ASSERT(CY_RSLT_SUCCESS == rslt1);

    // create the task
    BaseType_t rslt = xTaskCreate(task_servo,
                                  "servo",
                                  configMINIMAL_STACK_SIZE,
                                  NULL,
                                  configMAX_PRIORITIES - 5,
                                  NULL);
    if (rslt == pdPASS) {
        task_print("servo task created\n\r");
    }
    else {
        task_print("servo task NOT created\n\r");
    }
}
