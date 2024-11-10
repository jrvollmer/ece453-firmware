/******************************************************************************
 * File Name:   app_bt_helpers.c
 *
 * Description: This file consists of the helper functions for custom BLE
 *              functionality,
 *
 *******************************************************************************/


/******************************************************************************
 * Header Files
 ******************************************************************************/
#include "app_bt_helpers.h"

/****************************************************************************
 * Function Definitions
 ***************************************************************************/
/**
 * @brief  Get the value of the Use Item characteristic
 * 
 * @note   This function will unset the characteristic value if set
 * 
 * @return uint8_t
 * The value of the Use Item characteristic
 */
uint8_t get_use_item()
{
    uint8_t use_item_val;
    memcpy(&use_item_val, &app_rc_controller_use_item, app_rc_controller_use_item_len);
    // Reset use_item characteristic if set
    if (use_item_val)
    {
        const uint8_t zero = 0;
        memcpy(&app_rc_controller_use_item, &zero, app_rc_controller_use_item_len);
    }

    return use_item_val;
}

/* END OF FILE [] */
