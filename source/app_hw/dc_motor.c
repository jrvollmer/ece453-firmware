#include "dc_motor.h"

cyhal_pwm_t dc_pwm_obj;

/**
 * @brief
 * Initializes software resources related to the operation of
 * the brushless DC motor.  
 */
void dc_motor_init(void) {
	cy_rslt_t rslt;
    
    /* Initialize PWM on the supplied pin and assign a new clock */
    rslt = cyhal_pwm_init(&dc_pwm_obj, DC_MOTOR_PWM_PIN, NULL);
    CY_ASSERT(CY_RSLT_SUCCESS == rslt);

    // Initialize the pin that controls the direction
    rslt = cyhal_gpio_configure(DC_MOTOR_DIR_PIN, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG);
    CY_ASSERT(CY_RSLT_SUCCESS == rslt);
}

void set_dc_motor_direction(uint8_t dir) {
	cyhal_gpio_write(DC_MOTOR_DIR_PIN, dir);
}

void set_dc_motor_duty_cycle(uint8_t duty_cycle) {
	cy_rslt_t rslt;
	rslt = cyhal_pwm_set_duty_cycle(&dc_pwm_obj, 100-duty_cycle, 50);
	rslt = cyhal_pwm_start(&dc_pwm_obj);
	CY_ASSERT(CY_RSLT_SUCCESS == rslt);
}

void turn_dc_motor_off() {
	cy_rslt_t rslt;
	/* To turn off, must use PWM duty cycle of 100 (inverted so actually 0)*/
	rslt = cyhal_pwm_set_duty_cycle(&dc_pwm_obj, 100, 50);
	rslt = cyhal_pwm_start(&dc_pwm_obj);
	CY_ASSERT(CY_RSLT_SUCCESS == rslt);
}


