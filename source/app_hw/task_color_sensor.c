/**
 * @file task_color_sensor.c
 * @author Bowen Quan Team 01
 * @brief
 * @version 0.1
 * @date 2024-10-28
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "task_color_sensor.h"

QueueHandle_t q_color_sensor;

/** Write a register on the Color Sensor
 *
 * @param reg The reg address to read
 * @param value The value to be written
 *
 */
static void color_sensor_write_reg(uint8_t reg, uint16_t value)
{
	cy_rslt_t rslt;
	uint8_t write_buffer[3];

	write_buffer[0] = reg;
	write_buffer[1] = value & 0xFF; // low byte
    write_buffer[2] = value >> 8; // high byte

	/* There may be multiple I2C devices on the same bus, so we must take
	   the semaphore used to ensure mutual exclusion before we begin any
	   I2C transactions
	 */
	xSemaphoreTake(Semaphore_I2C, portMAX_DELAY);

	/* Use cyhal_i2c_master_write to write the required data to the device. */
	rslt = cyhal_i2c_master_write(
		&i2c_master_obj,		  // I2C Object
		VEML3328SL_SUBORDINATE_ADDR, // I2C Address
		write_buffer,			  // Array of data to write
		3,						  // Number of bytes to write
		0,						  // Block until completed
		true);					  // Generate Stop Condition

	if (rslt != CY_RSLT_SUCCESS) {
		task_print_error("Error writing to the Color Sensor\n");
	}

	/* Give up control of the I2C bus */
	xSemaphoreGive(Semaphore_I2C);
}

/** Read a register on the Color Sensor
 *
 * @param reg The reg address to read
 *
 */
static uint16_t color_sensor_read_reg(uint8_t reg) {
	cy_rslt_t rslt;

	uint8_t write_buffer[1];
	uint8_t read_buffer[2];

	write_buffer[0] = reg;

	/* There may be multiple I2C devices on the same bus, so we must take
	   the semaphore used to ensure mutual exclusion before we begin any
	   I2C transactions
	 */
	xSemaphoreTake(Semaphore_I2C, portMAX_DELAY);

	/* Use cyhal_i2c_master_write to write the required data to the device. */
	/* Send the register address, do not generate a stop condition.  This will result in */
	/* a restart condition. */
	rslt = cyhal_i2c_master_write(
		&i2c_master_obj,
		VEML3328SL_SUBORDINATE_ADDR, // I2C Address
		write_buffer,			  // Array of data to write
		1,						  // Number of bytes to write
		0,						  // Block until completed
		false);					  // Do NOT generate Stop Condition

	if (rslt != CY_RSLT_SUCCESS) {
		task_print("Error writing reg address to the Color Sensor");
	} else {
		/* Use cyhal_i2c_master_read to read the required data from the device. */
		// The register address has already been set in the write above, so read a single byte
		// of data.
		rslt = cyhal_i2c_master_read(
			&i2c_master_obj,		  // I2C Object
			VEML3328SL_SUBORDINATE_ADDR, // I2C Address
			read_buffer,			  // Read Buffer
			2,						  // Number of bytes to read
			0,						  // Block until completed
			true);					  // Generate Stop Condition

		if (rslt != CY_RSLT_SUCCESS) {
			task_print_error("Error Reading from the Color Sensor");
			read_buffer[0] = 0xFF;
            read_buffer[1] = 0xFF;
		}
	}

	/* Give up control of the I2C bus */
	xSemaphoreGive(Semaphore_I2C);

	return read_buffer[0] + (read_buffer[1] << 8);
}


/******************************************************************************/
/* Public Function Definitions                                                */
/******************************************************************************/

/**
 * @brief
 * Task that reads color sensor RGB registers, determines which color is being read,
 * and sends that info to the color sensor queue. 
 * @param param
 * Unused
 */
void task_color_sensor(void *param)
{
	while (1) {
		// read red, green, blue registers
		float red = (float)color_sensor_read_reg(COLOR_RED_REG);
		float green = (float)color_sensor_read_reg(COLOR_GREEN_REG) / 2; // green register has its value doubled compared to red and blue
		float blue = (float)color_sensor_read_reg(COLOR_BLUE_REG);
		float max = red;
		if (green > max) {
			max = green;
		} 
		if (blue > max) {
			max = blue;
		}
		// normalize the RGB values to a percentage
		red /= max;
		green /= max;
		blue /= max;
	
		// determine which track the color sensor sees
		color_sensor_terrain_t terrain = TRANSITION;
		if (red > blue + THRESHOLD && blue > green + 0.05) {
			terrain = PINK;
		} else if (red > green + THRESHOLD && red > blue + CARDBOARD_RB_THRESHOLD) {
			terrain = BROWN_ROAD;
		} else if (green > red + THRESHOLD && green > blue + THRESHOLD) {
			terrain = GREEN_GRASS;
		} else if (red > WHITE_THRESHOLD && green > WHITE_THRESHOLD && blue > WHITE_THRESHOLD) {
			terrain = WHITE;
		}

		if (terrain != TRANSITION) {
			// send to queue
			xQueueSend(q_color_sensor, &terrain, portMAX_DELAY);
		} else {
			vTaskDelay(pdMS_TO_TICKS(5));
		}
	}
}

/**
 * @brief
 * Initializes the Color Sensor.
 * You MUST call i2c_init(); before calling this function.
 */
void task_color_sensor_init(void) {
	q_color_sensor = xQueueCreate(1, sizeof(color_sensor_terrain_t));

	// turn on the color sensor
	color_sensor_write_reg(COLOR_CONFIG_REG, COLOR_TURN_ON_CMD);

	/* Create the task that will read values from the color sensor */
	xTaskCreate(
		task_color_sensor,
		"Task Color Sensor",
		configMINIMAL_STACK_SIZE*5, // TODO This can go much lower
		NULL,
		configMAX_PRIORITIES - 5,
		NULL);
}
