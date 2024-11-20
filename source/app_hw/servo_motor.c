#include "servo_motor.h"

cyhal_pwm_t servo_pwm_obj;


/**
 * @brief
 * Initializes software resources related to the operation of
 * the brushed DC motor.  
 */
void servo_motor_init(void) {
	 cy_rslt_t rslt;
    
    /* Initialize PWM on the supplied pin and assign a new clock */
     rslt = cyhal_pwm_init(&servo_pwm_obj, SERVO_PIN, NULL);
     CY_ASSERT(CY_RSLT_SUCCESS == rslt);
    
}

void set_servo_motor_duty_cycle(float duty_cycle) {
    cy_rslt_t rslt;
    rslt = cyhal_pwm_set_duty_cycle(&servo_pwm_obj, duty_cycle, 50);
    rslt = cyhal_pwm_start(&servo_pwm_obj);
    CY_ASSERT(CY_RSLT_SUCCESS == rslt);
}