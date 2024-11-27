/**
 * @file task_hall_sensor.c
 * @author Bowen Quan Team 01
 * @brief
 * @version 0.1
 * @date 2024-10-28
 *
 * @copyright Copyright (c) 2024
 *
 */

#include "task_hall_sensor.h"

// ADC objects
cyhal_adc_t adc_obj;
cyhal_adc_channel_t channel_obj;
const cyhal_adc_channel_config_t channel_config = { 
    .enable_averaging = false,  // disable averaging 
    .min_acquisition_ns = 220,  // minimum time this channel should be sampled
    .enabled = true            // this channel should be sampled when ADC scans
};

const cyhal_adc_config_t config = {
	.continuous_scanning = false,
	.average_count = 1,
	.vref= CYHAL_ADC_REF_VDDA,
	.vneg= CYHAL_ADC_VNEG_VSSA,
	.resolution=12u,
	.ext_vref= NC,
	.bypass_pin = NC
};

/**
 * @brief
 * Task used to monitor the reception of command packets sent the Hall Effect Sensor
 * @param param
 * Unused
 */
void task_hall_sensor(void *param) {
    // do an initial configuration value 
    uint32_t no_magnet_value = cyhal_adc_read(&channel_obj);
	while (1) {
		// read the ADC value
		uint32_t hall_sensor_out = cyhal_adc_read(&channel_obj);

        

		// determine if there is a magnet
		if(hall_sensor_out > no_magnet_value + HALL_THRESHOLD || hall_sensor_out < no_magnet_value - HALL_THRESHOLD) {
            app_bt_car_complete_lap(); // increment laps on mobile app
            vTaskDelay(5000); // prevent another lap from being scored for 5 secs
        }
        vTaskDelay(5);
	}
}

/**
 * @brief
 * Initializes software resources related to the operation of
 * the ADC.
 */
void task_hall_sensor_init(void)
{
    // initialize ADC
    cy_rslt_t rslt;
    rslt = cyhal_adc_init(&adc_obj, HALL_SENSOR_ADC_PIN, NULL);
    CY_ASSERT(rslt == CY_RSLT_SUCCESS);

	// configure adc to have VREF = 3.3V and VNEG = GND
	rslt = cyhal_adc_configure(&adc_obj, &config);
    CY_ASSERT(rslt == CY_RSLT_SUCCESS);

    rslt = cyhal_adc_channel_init_diff(
        &channel_obj, 
        &adc_obj, 
        HALL_SENSOR_ADC_PIN, 
        CYHAL_ADC_VNEG,                 // compare Pin with default vminus
        &channel_config 
    );
	CY_ASSERT(rslt == CY_RSLT_SUCCESS);


	/* Create the task that will control the status LED */
	BaseType_t rslt2 = xTaskCreate(
		task_hall_sensor,
		"Task Hall Effect Sensor",
		configMINIMAL_STACK_SIZE*3,
		NULL,
		configMAX_PRIORITIES - 5,
		NULL);
    if (rslt2 == pdPASS) {
        task_print("HALL task created\n\r");
    }
    else {
        task_print("HALL task NOT created\n\r");
    }
}
