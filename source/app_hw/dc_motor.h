/**
 * @file task_dc_brushless_control.h
 * @author Sarah Gerovac (sgerovac@wisc.edu) (Team 01)
 * @brief This is the header file that uses a direction input to 
 * control a DC brushless motor. It collects a user's input to rotate the motor 
 * forward or reverse, and provides feedback on which direction the motor is 
 * rotating or whether an invalid input was provided.
 * @version 0.1
 * @date 2024-10-21
 *
 * @copyright Copyright (c) 2024
 *
 */
#ifndef __DC_MOTOR_H__
#define __DC_MOTOR_H__

#include <stdint.h>
#include "cyhal_hw_types.h"
#include "cyhal_gpio.h"
#include "cy_result.h"
#include "cyhal.h"

// Pin controlling PWM signal
#define DC_MOTOR_PWM_PIN P9_0
// Pin controlling direction signal
#define DC_MOTOR_DIR_PIN P9_1

#define REVERSE 0
#define FORWARD 1

void dc_motor_init(void);
void set_dc_motor_direction(uint8_t dir);
void set_dc_motor_duty_cycle(uint8_t duty_cycle);
void turn_dc_motor_off();

#endif
