#include "servo_motor.h"
#include <math.h>

cyhal_pwm_t servo_pwm_obj;


void set_servo_motor_duty_cycle(float duty_cycle) {
    cy_rslt_t rslt;
    rslt = cyhal_pwm_set_duty_cycle(&servo_pwm_obj, duty_cycle, 50);
    rslt = cyhal_pwm_start(&servo_pwm_obj);
    CY_ASSERT(CY_RSLT_SUCCESS == rslt);
}

void task_servo() {
    car_joystick_t x = 0;
    car_joystick_t prev_x = x;
    while (1) {
        xQueueReceive(q_ble_car_joystick_x, &x, portMAX_DELAY);
        if (fabs(x - prev_x) > 0.01) {
            // Clamp to 0 to make sure we don't get stuck in a near-straight position
            if (x > -0.01 && x < 0.01) {
                x = 0;
            }
            // Servo is upside down, so left and right are reversed
            set_servo_motor_duty_cycle(STRAIGHT + TURN_DUTY_RANGE * x);
            prev_x = x;
        }
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
