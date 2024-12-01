/**
 * @file app_ir_led.c
 * @author James Vollmer (jrvollmer@wisc.edu) - Team 01
 * @brief 
 * Source file for IR LED interface
 * 
 * @version 0.1
 * @date 2024-11-16
 * 
 * @copyright Copyright (c) 2024
 */
#include "app_ir_led.h"
#include <timers.h>
#include "cy_pdl.h"
#include "cybsp.h"
#include "app_bt_car.h"


/*******************************************************************************
 * Function Prototypes
 ********************************************************************************/
static void app_ir_led_timers_init(void);
static void app_ir_led_tcpwm_init(void);


/******************************************************************************
 * Global Variables                                                           *
 ******************************************************************************/
static uint8_t n_active_timers = 0;
static TimerHandle_t ir_led_timers[CAR_ITEM_MAX];
static const uint32_t item_delays_ms[CAR_ITEM_MAX] = {
    [CAR_ITEM_SHOT]   = 1000,
    [CAR_ITEM_SHIELD] = 0,
    [CAR_ITEM_BOOST]  = 0,
};


/*******************************************************************************
 * Function Definitions
 *******************************************************************************/
/**
 * @brief Stop IR LED after all active timers expire
 * 
 * @param TimerHandle_t
 * (Unused) Timer that has expired
 * @return void
 */
static void app_ir_led_stop_ir_led(TimerHandle_t xTimer)
{
    // Suppress unused parameter warning
    (void)xTimer;

    Cy_TCPWM_TriggerStopOrKill(TCPWM0, tcpwm_0_cnt_5_MASK);
}


/**
 * @brief Pulse IR LED based on the specified item
 * 
 * @param car_item_t
 * Item to use. Controls the time for which the IR LED is pulsed
 * 
 * @return BaseType_t
 * pdTRUE if IR LED started successfully, pdFALSE otherwise
 */
BaseType_t app_ir_led_use_item(car_item_t item)
{
    BaseType_t ret = pdFALSE;

    if ((item > CAR_ITEM_MIN) && (item < CAR_ITEM_MAX) && (item_delays_ms[item] > 0))
    {
        if (!xTimerIsTimerActive(ir_led_timers[item]))
        {
            // Pulse the IR LED at 38kHz
            Cy_TCPWM_TriggerStart(TCPWM0, tcpwm_0_cnt_5_MASK);

            // TODO TODO Just use vTaskDelay(pdMS_TO_TICKS(item_delays_ms[item])); in task_ir_led and then call app_ir_led_start/stop_ir_led

            xTimerStart(ir_led_timers[item], 0);
            n_active_timers++;
        }
        else
        {
            xTimerReset(ir_led_timers[item], 0);
        }

        ret = pdTRUE;
    }

    return ret;
}


/**
 * @brief Initialize timers for IR LED stop conditions
 * 
 * @param void
 * @return void
 */
static void app_ir_led_timers_init(void)
{
    for (int i=(CAR_ITEM_MIN+1); i < CAR_ITEM_MAX; i++)
    {
        if (item_delays_ms[i] > 0)
        {
            ir_led_timers[i] = xTimerCreate("IR_LED_Stop_Timer",
                                            pdMS_TO_TICKS(item_delays_ms[i]),
                                            pdFALSE,
                                            (void*)0,
                                            app_ir_led_stop_ir_led);
        }
    }
}


/**
 * @brief Initialize TCPWM for IR LED
 * 
 * @param void
 * @return void
 */
static void app_ir_led_tcpwm_init(void)
{
    // NOTE: The TCPWM clock is set using the Device Configurator tool
    Cy_TCPWM_PWM_Init(tcpwm_0_cnt_5_HW, tcpwm_0_cnt_5_NUM, &tcpwm_0_cnt_5_config);
    Cy_TCPWM_PWM_Enable(tcpwm_0_cnt_5_HW, tcpwm_0_cnt_5_NUM);
}


void app_ir_led_init(void)
{
    app_ir_led_timers_init();
    app_ir_led_tcpwm_init();
}

/* [] END OF FILE */