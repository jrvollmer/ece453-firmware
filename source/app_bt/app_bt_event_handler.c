/*******************************************************************************
 * File Name: app_bt_event_handler.c
 *
 * Description: This file contains the bluetooth event handler that processes
 *              the bluetooth events from host stack. 
 *
 * Related Document: See README.md
 *
 *
 *******************************************************************************
 * Copyright 2021-2024, Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
 *
 * This software, including source code, documentation and related
 * materials ("Software") is owned by Cypress Semiconductor Corporation
 * or one of its affiliates ("Cypress") and is protected by and subject to
 * worldwide patent protection (United States and foreign),
 * United States copyright laws and international treaty provisions.
 * Therefore, you may use this Software only as provided in the license
 * agreement accompanying the software package from which you
 * obtained this Software ("EULA").
 * If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
 * non-transferable license to copy, modify, and compile the Software
 * source code solely for use in connection with Cypress's
 * integrated circuit products.  Any reproduction, modification, translation,
 * compilation, or representation of this Software except as specified
 * above is prohibited without the express written permission of Cypress.
 *
 * Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
 * reserves the right to make changes to the Software without notice. Cypress
 * does not assume any liability arising out of the application or use of the
 * Software or any product or circuit described in the Software. Cypress does
 * not authorize its products for use in any products where a malfunction or
 * failure of the Cypress product may reasonably be expected to result in
 * significant property damage, injury or death ("High Risk Product"). By
 * including Cypress's product in a High Risk Product, the manufacturer
 * of such system or application assumes all risk of such use and in doing
 * so agrees to indemnify Cypress against all liability.
 ******************************************************************************/

/*******************************************************************************
 * Header Files
 ******************************************************************************/

#include "inttypes.h"
#include <FreeRTOS.h>
#include <task.h>
#include "app_bt_bonding.h"
#include "app_flash_common.h"
#include "cycfg_gap.h"
#include "app_bt_utils.h"
#include "app_hw_device.h"
#include "app_bt_event_handler.h"
#include "app_bt_gatt_handler.h"
#include "app_bt_utils.h"
#include "app_bt_car.h"
#include "wiced_bt_stack.h"
#include "wiced_bt_dev.h"
#include "wiced_bt_ble.h"
#ifdef ENABLE_BT_SPY_LOG
#include "cybt_debug_uart.h"
#endif

/*******************************************************************************
 * Variable Definitions
 ******************************************************************************/

/* Holds important BLE state information that is not provided by generated sources */
ble_state_t ble_state;


/* This is the index for the link keys, cccd and privacy mode of the host we are 
 * currently bonded to */
uint8_t  bondindex = 0;

/*******************************************************************************
 * Function Definitions
 ******************************************************************************/

/**
 * Function Name: app_bt_management_callback
 *
 * Function Description:
 *   @brief This is a Bluetooth stack event handler function to receive
 *   management events from the LE stack and process as per the application.
 *
 *   @param wiced_bt_management_evt_t event             : LE event code of
 *          one byte length
 *   @param wiced_bt_management_evt_data_t *p_event_data: Pointer to LE
 *          management event structures
 *
 *   @return wiced_result_t                             : Error code from
 *           WICED_RESULT_LIST or BT_RESULT_LIST
 *
 */
wiced_result_t
app_bt_management_callback(wiced_bt_management_evt_t event, wiced_bt_management_evt_data_t *p_event_data)
{
    wiced_result_t result = WICED_BT_SUCCESS;
    cy_rslt_t rslt;
    wiced_bt_dev_encryption_status_t *p_status;
    wiced_bt_ble_advert_mode_t *p_mode;
    wiced_bt_dev_ble_pairing_info_t *p_info;
    wiced_bt_device_address_t local_bda = {0x00, 0xA0, 0x50,
                                           0x011, 0x44, 0x55};

    UNUSED_VARIABLE(p_info);

    switch (event)
    {
        case BTM_ENABLED_EVT:
            /* Bluetooth Controller and Host Stack Enabled */
            if (WICED_BT_SUCCESS == p_event_data->enabled.status)
            {
                wiced_bt_set_local_bdaddr(local_bda, BLE_ADDR_PUBLIC);
                wiced_bt_dev_read_local_addr(local_bda);

                /* Perform application-specific initialization */
                app_bt_application_init();
            }
            break;

        case BTM_DISABLED_EVT:
            break;

        case BTM_PAIRING_IO_CAPABILITIES_BLE_REQUEST_EVT:
            p_event_data->pairing_io_capabilities_ble_request.local_io_cap  =  \
            BTM_IO_CAPABILITIES_NONE;
            p_event_data->pairing_io_capabilities_ble_request.oob_data      =  \
            BTM_OOB_NONE;
            p_event_data->pairing_io_capabilities_ble_request.auth_req      =  \
            BTM_LE_AUTH_REQ_SC_BOND;
            p_event_data->pairing_io_capabilities_ble_request.max_key_size  =  \
            0x10;
            p_event_data->pairing_io_capabilities_ble_request.init_keys     =  \
            BTM_LE_KEY_PENC|BTM_LE_KEY_PID|BTM_LE_KEY_PCSRK|BTM_LE_KEY_LENC;
            p_event_data->pairing_io_capabilities_ble_request.resp_keys     =  \
            BTM_LE_KEY_PENC|BTM_LE_KEY_PID|BTM_LE_KEY_PCSRK|BTM_LE_KEY_LENC;
            break;

        case BTM_PAIRING_COMPLETE_EVT:
            p_info = &p_event_data->pairing_complete.pairing_complete_info.ble;
            /* Update Num of bonded devices and next free slot in slot data*/
            rslt = app_bt_update_slot_data();
            break;


        case BTM_BLE_ADVERT_STATE_CHANGED_EVT:
            p_mode = &p_event_data->ble_advert_state_changed;
            if (BTM_BLE_ADVERT_OFF == *p_mode)
            {
                app_bt_adv_stop_handler();
            }
            break;

        case BTM_PAIRED_DEVICE_LINK_KEYS_UPDATE_EVT:
            /* save device keys to NVRAM */
            rslt = app_bt_save_device_link_keys(&(p_event_data->paired_device_link_keys_update));
            break;

        case  BTM_PAIRED_DEVICE_LINK_KEYS_REQUEST_EVT:
            /* Paired Device Link Keys Request */

            /* Need to search to see if the BD_ADDR we are
             * looking for is in NVRAM. If not, we return WICED_BT_ERROR
             * and the stack will generate keys and will then call
             * BTM_PAIRED_DEVICE_LINK_KEYS_UPDATE_EVT so that they
             * can be stored
             */

            /* Assume the device won't be found.
             * If it is, we will set this back to WICED_BT_SUCCESS */
            result = WICED_BT_ERROR;
            bondindex = app_bt_find_device_in_flash(p_event_data->paired_device_link_keys_request.bd_addr);
            if(BOND_INDEX_MAX > bondindex)
            {
                /* Copy the keys to where the stack wants it */
                memcpy(&(p_event_data->paired_device_link_keys_request),
                       &bond_info.link_keys[bondindex],
                       sizeof(wiced_bt_device_link_keys_t));
                result = WICED_BT_SUCCESS;
            }
            break;

        case BTM_LOCAL_IDENTITY_KEYS_UPDATE_EVT:
            /* Update of local privacy keys - save to NVRAM */
            rslt = app_bt_save_local_identity_key(p_event_data->local_identity_keys_update);
            if (CY_RSLT_SUCCESS != rslt)
            {
                result = WICED_BT_ERROR;
            }
            break;

        case  BTM_LOCAL_IDENTITY_KEYS_REQUEST_EVT:
            app_kv_store_init();
            /* Read Local Identity Resolution Keys if present in NVRAM*/
            rslt = app_bt_read_local_identity_keys();
            if(CY_RSLT_SUCCESS == rslt)
            {
                memcpy(&(p_event_data->local_identity_keys_request),
                       &(identity_keys), sizeof(wiced_bt_local_identity_keys_t));
                result = WICED_BT_SUCCESS;
            }
            else
            {
                result = WICED_BT_ERROR;
            }
            break;

        case BTM_ENCRYPTION_STATUS_EVT:
            p_status = &p_event_data->encryption_status;
            /* Check and retreive the index of the bond data of the device that
             * got connected */
            /* This call will return BOND_INDEX_MAX if the device is not found */
            bondindex = app_bt_find_device_in_flash(p_event_data->encryption_status.bd_addr);
            if(bondindex < BOND_INDEX_MAX)
            {
                app_bt_restore_bond_data();
                app_bt_restore_cccd();
                /* Set CCCD value from the value that was previously saved in the NVRAM */
                app_rc_controller_get_item_client_char_config[0] = peer_cccd_data[bondindex];
                app_rc_controller_lap_client_char_config[0] = peer_cccd_data[bondindex];
            }
            else
            {
                bondindex=0;
            }
            break;

        case BTM_SECURITY_REQUEST_EVT:
            wiced_bt_ble_security_grant(p_event_data->security_request.bd_addr,
                                        WICED_BT_SUCCESS);
            break;

        case BTM_BLE_CONNECTION_PARAM_UPDATE:
            /* No action */
            break;

        case BTM_BLE_PHY_UPDATE_EVT:
            /* No action */
            break;

        default:
            /* No action */
            break;
    }
    return result;
}

/**
 * Function Name: hci_trace_cback
 *
 * Function Description:
 *   @brief This callback routes HCI packets to debug uart.
 *
 *   @param wiced_bt_hci_trace_type_t type : HCI trace type
 *   @param uint16_t length : length of p_data
 *   @param uint8_t* p_data : pointer to data
 *
 *   @return None
 *
 */
#ifdef ENABLE_BT_SPY_LOG
void hci_trace_cback(wiced_bt_hci_trace_type_t type,
                     uint16_t length, uint8_t* p_data)
{
    cybt_debug_uart_send_hci_trace(type, length, p_data);
}
#endif

/**
 * Function Name: app_bt_application_init
 *
 * Function Description:
 *   @brief This function handles application level initialization tasks and is
 *   called from the BT management callback once the LE stack enabled event
 *   (BTM_ENABLED_EVT) is triggered. This function is executed in the
 *   BTM_ENABLED_EVT management callback.
 *
 *   @param None
 *
 *   @return None
 *
 */
void app_bt_application_init(void)
{
    wiced_result_t result = WICED_BT_SUCCESS;
    wiced_bt_gatt_status_t gatt_status = WICED_BT_GATT_SUCCESS;

    UNUSED_VARIABLE(gatt_status);

    #ifdef ENABLE_BT_SPY_LOG
    wiced_bt_dev_register_hci_trace(hci_trace_cback);
    #endif

    app_bt_interrupt_config();

    // Initialize application-level BLE resources for the car
    app_bt_car_init();
    // Initialize all HW resources that interface with BLE
    app_bt_hw_init();

    if(CY_RSLT_SUCCESS == app_bt_restore_bond_data())
    {
        /* Load previous paired keys for address resolution */
        app_bt_add_devices_to_address_resolution_db();
    }

    /* Allow peer to pair */
    wiced_bt_set_pairable_mode(WICED_TRUE, FALSE); // TODO Try adding this into the disconnect logic as well. Also, try only allowing connected devices to pair

    /* Set Advertisement Data */
    wiced_bt_ble_set_raw_advertisement_data(CY_BT_ADV_PACKET_DATA_SIZE,
                                            cy_bt_adv_packet_data);

    /* Register with BT stack to receive GATT callback */
    gatt_status = wiced_bt_gatt_register(app_bt_gatt_callback);

    /* Initialize GATT Database */
    gatt_status = wiced_bt_gatt_db_init(gatt_database, gatt_database_len, NULL);

    /* Start Undirected LE Advertisements on device startup.
     * The corresponding parameters are contained in 'app_bt_cfg.c' */
    result = wiced_bt_start_advertisements(BTM_BLE_ADVERT_UNDIRECTED_HIGH,
                                           0,
                                           NULL);

    /* Failed to start advertisement. Stop program execution */
    if (WICED_BT_SUCCESS != result)
    {
        CY_ASSERT(0);
    }
}

/**
 * Function Name: app_bt_adv_stop_handler
 *
 * Function Description:
 *   @brief This function handles advertisement stop event.
 *
 *   @param None
 *
 *   @return None
 *
 */
void app_bt_adv_stop_handler(void)
{
    wiced_result_t result;

    if ((!ble_state.conn_id) && (!pairing_mode))
    {
        result =  wiced_bt_start_advertisements(BTM_BLE_ADVERT_UNDIRECTED_HIGH,
                                                0,
                                                NULL);
    }
    UNUSED_VARIABLE(result);
}

/* END OF FILE [] */
