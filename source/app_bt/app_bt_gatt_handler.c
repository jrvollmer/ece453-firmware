
/*******************************************************************************
 * File Name: app_bt_gatt_handler.c
 *
 * Editor: James Vollmer (jrvollmer@wisc.edu) - Team 01
 * Description: This file contains the task that handles GATT events.
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
#include "wiced_memory.h"
#include "wiced_bt_stack.h"
#include "wiced_bt_dev.h"
#include "wiced_bt_ble.h"
#include "app_bt_bonding.h"
#include "app_flash_common.h"
#include "cycfg_gap.h"
#include "app_bt_utils.h"
#include "app_bt_event_handler.h"
#include "app_bt_gatt_handler.h"
#include "app_hw_device.h"
#include "app_bt_car.h"
#ifdef ENABLE_BT_SPY_LOG
#include "cybt_debug_uart.h"
#endif

/*******************************************************************************
 * Function Definitions
 ******************************************************************************/
/**
 * Function Name: app_bt_gatt_event_callback
 *
 * Function Description:
 *   @brief This function handles GATT events from the BT stack.
 *
 *   @param wiced_bt_gatt_evt_t event                : LE GATT event code of one
 *          byte length
 *   @param wiced_bt_gatt_event_data_t *p_event_data : Pointer to LE GATT event
 *                                                    structures
 *
 *   @return wiced_bt_gatt_status_t                  : See possible status
 *           codes in wiced_bt_gatt_status_e in wiced_bt_gatt.h
 *
 */
wiced_bt_gatt_status_t
app_bt_gatt_callback(wiced_bt_gatt_evt_t event,
                            wiced_bt_gatt_event_data_t *p_event_data)
{
    wiced_bt_gatt_status_t gatt_status = WICED_BT_SUCCESS;

    uint16_t error_handle = 0;

    wiced_bt_gatt_attribute_request_t *p_attr_req = &p_event_data->attribute_request;
    /* Call the appropriate callback function based on the GATT event type,
     * and pass the relevant event parameters to the callback function */
    switch (event)
    {
        case GATT_CONNECTION_STATUS_EVT:
            gatt_status = app_bt_gatt_conn_status_cb( &p_event_data->connection_status );
            break;

        case GATT_ATTRIBUTE_REQUEST_EVT:
            gatt_status = app_bt_gatt_req_cb(p_attr_req, 
                                             &error_handle);
            if(gatt_status!=WICED_BT_GATT_SUCCESS)
            {
               wiced_bt_gatt_server_send_error_rsp(p_attr_req->conn_id, 
                                                          p_attr_req->opcode, 
                                                          error_handle, 
                                                          gatt_status);         
            }
            break; 
        case GATT_GET_RESPONSE_BUFFER_EVT:
            p_event_data->buffer_request.buffer.p_app_rsp_buffer =
            app_bt_alloc_buffer(p_event_data->buffer_request.len_requested);
            p_event_data->buffer_request.buffer.p_app_ctxt = (void *)app_bt_free_buffer;
            gatt_status = WICED_BT_GATT_SUCCESS;
            break;
            /* GATT buffer transmitted event,
             * check \ref wiced_bt_gatt_buffer_transmitted_t*/
        case GATT_APP_BUFFER_TRANSMITTED_EVT:
        {
            pfn_free_buffer_t pfn_free =                                       \
            (pfn_free_buffer_t)p_event_data->buffer_xmitted.p_app_ctxt;

            /* If the buffer is dynamic, the context will point to a function
             * to free it. */
            if (pfn_free)
                pfn_free(p_event_data->buffer_xmitted.p_app_data);

            gatt_status = WICED_BT_GATT_SUCCESS;
        }
            break;


        default:
            gatt_status = WICED_BT_GATT_ERROR;
               break;
    }

    return gatt_status;
}

/**
 * Function Name: app_bt_gatt_req_cb
 *
 * Function Description:
 *   @brief This function handles GATT server events from the BT stack.
 *
 * @param p_attr_req             : Pointer to LE GATT connection status
 *
 * @return wiced_bt_gatt_status_t: See possible status codes in
 *                                 wiced_bt_gatt_status_e in wiced_bt_gatt.h
 *
 */
wiced_bt_gatt_status_t
app_bt_gatt_req_cb (wiced_bt_gatt_attribute_request_t *p_attr_req, 
                    uint16_t *p_error_handle)
{
    wiced_bt_gatt_status_t gatt_status = WICED_BT_SUCCESS;
    switch ( p_attr_req->opcode )
    {
        case GATT_REQ_READ:
        case GATT_REQ_READ_BLOB:
             /* Attribute read request */
            gatt_status =                                                      \
            app_bt_gatt_req_read_handler(p_attr_req->conn_id, 
                                         p_attr_req->opcode, 
                                         &p_attr_req->data.read_req, 
                                         p_attr_req->len_requested, 
                                         p_error_handle);
            break;

        case GATT_REQ_WRITE:
        case GATT_CMD_WRITE:
            /* Attribute write request */
            gatt_status =
            app_bt_gatt_req_write_handler(p_attr_req->conn_id, 
                                          p_attr_req->opcode, 
                                          &p_attr_req->data.write_req, 
                                          p_attr_req->len_requested, 
                                          p_error_handle);

            if ((GATT_REQ_WRITE == p_attr_req->opcode) &&
                (WICED_BT_GATT_SUCCESS == gatt_status))
            {
                wiced_bt_gatt_write_req_t *p_write_request = &p_attr_req->data.write_req;
                wiced_bt_gatt_server_send_write_rsp(p_attr_req->conn_id,
                                                    p_attr_req->opcode,
                                                    p_write_request->handle);
            }
            break;

        case GATT_REQ_MTU:
            gatt_status =                                                      \
            wiced_bt_gatt_server_send_mtu_rsp(p_attr_req->conn_id,
                                              p_attr_req->data.remote_mtu,
                                              CY_BT_MTU_SIZE);
            break;

        case GATT_HANDLE_VALUE_NOTIF:
            break;

        case GATT_REQ_READ_BY_TYPE:
            gatt_status =                                                      \
            app_bt_gatt_req_read_by_type_handler(p_attr_req->conn_id, 
                                                 p_attr_req->opcode, 
                                                 &p_attr_req->data.read_by_type, 
                                                 p_attr_req->len_requested, 
                                                 p_error_handle);
            break;

        case GATT_HANDLE_VALUE_CONF:
            {
                ble_state.flag_indication_sent = FALSE;
            }
            break;

        default:
            gatt_status = WICED_BT_GATT_ERROR;
            break;
    }

    return gatt_status;
}

/**
 * Function Name: app_bt_gatt_conn_status_cb
 *
 * Function Description:
 *   @brief This callback function handles connection status changes.
 *
 *   @param wiced_bt_gatt_connection_status_t *p_conn_status :
 *          Pointer to data that has connection details
 *
 *   @return wiced_bt_gatt_status_t                          : See possible
 *    status codes in wiced_bt_gatt_status_e in wiced_bt_gatt.h
 *
 */
wiced_bt_gatt_status_t
app_bt_gatt_conn_status_cb(wiced_bt_gatt_connection_status_t *p_conn_status)
{
    if (p_conn_status->connected)
    {
        return app_bt_gatt_connection_up(p_conn_status);
    }
    else
    {
        return app_bt_gatt_connection_down(p_conn_status);
    }
}

/**
 * Function Name: app_bt_gatt_req_read_handler
 *
 * Function Description:
 *   @brief This function handles Read Requests received from the client device
 *
 *   @param conn_id              : Connection ID
 *   @param opcode               : LE GATT request type opcode
 *   @param p_read_req           : Pointer to read request containing the handle
 *          to read
 *   @param len_req              : length of data requested
 *
 * @return wiced_bt_gatt_status_t: See possible status codes in
 *                                 wiced_bt_gatt_status_e in wiced_bt_gatt.h
 *
 */
wiced_bt_gatt_status_t
app_bt_gatt_req_read_handler(uint16_t conn_id, 
                             wiced_bt_gatt_opcode_t opcode, 
                             wiced_bt_gatt_read_t *p_read_req, 
                             uint16_t len_req, 
                             uint16_t *p_error_handle)
{

    gatt_db_lookup_table_t  *puAttribute;
    int          attr_len_to_copy;
    uint8_t     *from;
    int          to_send;

   *p_error_handle = p_read_req->handle;

    puAttribute = app_bt_find_by_handle(p_read_req->handle);
    if (NULL == puAttribute)
    {
        return WICED_BT_GATT_INVALID_HANDLE;
    }
    attr_len_to_copy = puAttribute->cur_len;

    if (p_read_req->offset >= puAttribute->cur_len)
    {
        return WICED_BT_GATT_INVALID_OFFSET;
    }

    to_send = MIN(len_req, attr_len_to_copy - p_read_req->offset);
    from = ((uint8_t *)puAttribute->p_data) + p_read_req->offset;
    /* No need for context, as buff not allocated */
    return wiced_bt_gatt_server_send_read_handle_rsp(conn_id,
                                                     opcode,
                                                     to_send,
                                                     from,
                                                     NULL);
}

/**
 * Function Name: app_bt_gatt_req_write_handler
 *
 * Function Description:
 *   @brief This function handles Write Requests received from the client device
 *
 *   @param conn_id                : Connection ID
 *   @param opcode                 : LE GATT request type opcode
 *   @param p_write_req            : Pointer to LE GATT write request
 *   @param len_req                : length of data requested
 *
 *   @return wiced_bt_gatt_status_t: See possible status codes in
 *                                   wiced_bt_gatt_status_e in wiced_bt_gatt.h
 *
 */
wiced_bt_gatt_status_t
app_bt_gatt_req_write_handler(uint16_t conn_id, 
                              wiced_bt_gatt_opcode_t opcode, 
                              wiced_bt_gatt_write_req_t *p_write_req, 
                              uint16_t len_req, 
                              uint16_t *p_error_handle)
{
    wiced_bt_gatt_status_t gatt_status = WICED_BT_GATT_INVALID_HANDLE;
   *p_error_handle = p_write_req->handle;

    /* Attempt to perform the Write Request */

    gatt_status = app_bt_set_value(p_write_req->handle,
                                   p_write_req->p_val,
                                   p_write_req->val_len);

    return (gatt_status);
}

/**
 * Function Name : app_bt_gatt_req_read_by_type_handler
 *
 * Function Description :
 *   @brief Process read-by-type request from peer device
 *
 *   @param uint16_t conn_id                       : Connection ID
 *   @param wiced_bt_gatt_opcode_t opcode          : LE GATT request type opcode
 *   @param wiced_bt_gatt_read_by_type_t p_read_req: Pointer to read request
 *          containing the handle to read
 *  @param uint16_t len_requested                  : Length of data requested
 *
 * Return:
 *  wiced_bt_gatt_status_t                         : LE GATT status
 */
wiced_bt_gatt_status_t
app_bt_gatt_req_read_by_type_handler(uint16_t conn_id, 
                                     wiced_bt_gatt_opcode_t opcode, 
                                     wiced_bt_gatt_read_by_type_t *p_read_req, 
                                     uint16_t len_requested, 
                                     uint16_t *p_error_handle)
{
    gatt_db_lookup_table_t *puAttribute;
    uint16_t last_handle = 0;
    uint16_t attr_handle = p_read_req->s_handle;
    uint8_t *p_rsp = app_bt_alloc_buffer(len_requested);
    uint8_t pair_len = 0;
    int used_len = 0;

    UNUSED_VARIABLE(last_handle);

    if (NULL == p_rsp)
    {
        return WICED_BT_GATT_INSUF_RESOURCE;
    }

    /* Read by type returns all attributes of the specified type,
     * between the start and end handles */
    while (WICED_TRUE)
    {
       *p_error_handle = attr_handle;
        last_handle = attr_handle;
        attr_handle = wiced_bt_gatt_find_handle_by_type(attr_handle,
                                                        p_read_req->e_handle,
                                                        &p_read_req->uuid);
        if (0 == attr_handle)
            break;

        if ( NULL == (puAttribute = app_bt_find_by_handle(attr_handle)))
        {
            app_bt_free_buffer(p_rsp);
            return WICED_BT_GATT_INVALID_HANDLE;
        }

        int filled =
        wiced_bt_gatt_put_read_by_type_rsp_in_stream(p_rsp + used_len,
                                                     len_requested - used_len,
                                                     &pair_len,
                                                     attr_handle,
                                                     puAttribute->cur_len,
                                                     puAttribute->p_data);
        if (0 == filled)
        {
            break;
        }
        used_len += filled;

        /* Increment starting handle for next search to one past current */
        attr_handle++;
    }

    if (0 == used_len)
    {
        app_bt_free_buffer(p_rsp);
        return WICED_BT_GATT_INVALID_HANDLE;
    }

    /* Send the response */
    return wiced_bt_gatt_server_send_read_by_type_rsp(conn_id,
                                                      opcode,
                                                      pair_len,
                                                      used_len,
                                                      p_rsp,
                                                      (void *)app_bt_free_buffer);
}

/**
 * Function Name: app_bt_gatt_connection_up
 *
 * Function Description:
 *   @brief This function is invoked when connection is established
 *
 *   @param wiced_bt_gatt_connection_status_t *p_status :
 *          Pointer to data that has connection details
 *
 *   @return wiced_bt_gatt_status_t                     : See possible status
 *   codes in wiced_bt_gatt_status_e in wiced_bt_gatt.h
 *
 */
wiced_bt_gatt_status_t
app_bt_gatt_connection_up( wiced_bt_gatt_connection_status_t *p_status )
{
    /* Update the connection handler.  Save address of the connected device. */
    ble_state.conn_id = p_status->conn_id;
    memcpy(ble_state.remote_addr, p_status->bd_addr,
           sizeof(wiced_bt_device_address_t));
#ifdef PSOC6_BLE
    /* Refer to Note 2 in Document History section of Readme.md */
    if(pairing_mode == TRUE)
    {
        app_bt_add_devices_to_address_resolution_db();
        pairing_mode = FALSE;
    }
#endif
    /* Update the adv/conn state */
    return WICED_BT_GATT_SUCCESS;
}

/**
 * Function Name: app_bt_gatt_connection_down
 *
 * Function Description:
 *   @brief This function is invoked when connection is disconnected
 *
 *   @param wiced_bt_gatt_connection_status_t *p_status :
 *          Pointer to data that has connection details
 *
 *   @return wiced_bt_gatt_status_t                     : See possible status
 *           codes in wiced_bt_gatt_status_e in wiced_bt_gatt.h
 *
 */
wiced_bt_gatt_status_t
app_bt_gatt_connection_down(wiced_bt_gatt_connection_status_t *p_status)
{
    wiced_result_t result;

    UNUSED_VARIABLE(result);

    /* Resetting the device info */
    memset(ble_state.remote_addr, 0, BD_ADDR_LEN);
    ble_state.conn_id = 0;

    /* Start advertisements after disconnection */
    pairing_mode = TRUE;
    result = wiced_bt_start_advertisements(BTM_BLE_ADVERT_UNDIRECTED_HIGH,
                                           0,
                                           NULL);

    return WICED_BT_GATT_SUCCESS;
}

/**
 * Function Name : app_bt_find_by_handle
 *
 * Function Description:
 *   @brief Find attribute description by handle
 *
 *   @param uint16_t handle          : handle to look up
 *
 *   @return gatt_db_lookup_table_t  : pointer containing handle data
 *
 */
gatt_db_lookup_table_t  *app_bt_find_by_handle(uint16_t handle)
{
    int i;
    for (i = 0; i < app_gatt_db_ext_attr_tbl_size; i++)
    {
        if (handle == app_gatt_db_ext_attr_tbl[i].handle)
        {
            return (&app_gatt_db_ext_attr_tbl[i]);
        }
    }
    return NULL;
}

/**
 * Function Name: app_bt_set_value
 *
 * Function Description:
 *   @brief This function handles writing to the attribute handle in the GATT
 *   database using the data passed from the BT stack. The value to write is
 *   stored in a buffer whose starting address is passed as one of the
 *   function parameters
 *
 *   @param uint16_t attr_handle : GATT attribute handle
 *   @param uint8_t  p_val       : Pointer to LE GATT write request value
 *   @param uint16_t len         : length of GATT write request
 *
 *
 * @return wiced_bt_gatt_status_t: See possible status codes in
 *                                 wiced_bt_gatt_status_e in wiced_bt_gatt.h
 *
 */
wiced_bt_gatt_status_t app_bt_set_value(uint16_t attr_handle,
                                              uint8_t *p_val,
                                              uint16_t len)
{
    wiced_bool_t isHandleInTable = WICED_FALSE;
    wiced_bool_t validLen = WICED_FALSE;
    wiced_bt_gatt_status_t gatt_status = WICED_BT_GATT_INVALID_HANDLE;
    uint8_t *p_attr   = p_val;
    cy_rslt_t rslt;
    /* Check for a matching handle entry */
    for (int i = 0; i < app_gatt_db_ext_attr_tbl_size; i++)
    {
        if (app_gatt_db_ext_attr_tbl[i].handle == attr_handle)
        {
            /* Detected a matching handle in external lookup table */
            isHandleInTable = WICED_TRUE;

            /* Check if the buffer has space to store the data */
            validLen = (app_gatt_db_ext_attr_tbl[i].max_len >= len);

            if (validLen)
            {
                /* Value fits within the supplied buffer; copy over the value */
                app_gatt_db_ext_attr_tbl[i].cur_len = len;
                memcpy(app_gatt_db_ext_attr_tbl[i].p_data, p_val, len);
                gatt_status = WICED_BT_GATT_SUCCESS;

                switch (attr_handle)
                {

                case HDLC_RC_CONTROLLER_JOYSTICK_X_VALUE:
                    if (len != MAX_LEN_RC_CONTROLLER_JOYSTICK_X)
                    {
                        return WICED_BT_GATT_INVALID_ATTR_LEN;
                    }
                    app_rc_controller_joystick_x[0] = p_attr[0];
                    app_rc_controller_joystick_x[1] = p_attr[1];
                    app_rc_controller_joystick_x[2] = p_attr[2];
                    app_rc_controller_joystick_x[3] = p_attr[3];

                    // Queue float for consumption
                    car_joystick_t val_x;
                    memcpy(&val_x, &app_rc_controller_joystick_x, app_rc_controller_joystick_x_len);
                    xQueueSendToBack(q_ble_car_joystick_x, &val_x, 0);

                    break;

                case HDLC_RC_CONTROLLER_JOYSTICK_Y_VALUE:
                    if (len != MAX_LEN_RC_CONTROLLER_JOYSTICK_Y)
                    {
                        return WICED_BT_GATT_INVALID_ATTR_LEN;
                    }
                    app_rc_controller_joystick_y[0] = p_attr[0];
                    app_rc_controller_joystick_y[1] = p_attr[1];
                    app_rc_controller_joystick_y[2] = p_attr[2];
                    app_rc_controller_joystick_y[3] = p_attr[3];

                    // Queue float for consumption
                    car_joystick_t val_y;
                    memcpy(&val_y, &app_rc_controller_joystick_y, app_rc_controller_joystick_y_len);
                    xQueueSendToBack(q_ble_car_joystick_y, &val_y, 0);

                    break;

                case HDLC_RC_CONTROLLER_USE_ITEM_VALUE:
                    if (len != MAX_LEN_RC_CONTROLLER_USE_ITEM)
                    {
                        return WICED_BT_GATT_INVALID_ATTR_LEN;
                    }
                    app_rc_controller_use_item[0] = p_attr[0];

                    // Triggers IR LED and audio tasks
                    app_bt_car_use_item((car_item_t)app_rc_controller_use_item[0]);

                    break;

                /* By writing into Characteristic Client Configuration descriptor
                 * peer can enable or disable notification or indication */
                case HDLD_RC_CONTROLLER_GET_ITEM_CLIENT_CHAR_CONFIG:
                    if (len != 2)
                    {
                        return WICED_BT_GATT_INVALID_ATTR_LEN;
                    }
                    app_rc_controller_get_item_client_char_config[0] = p_attr[0];
                    peer_cccd_data[bondindex] = p_attr[0] | (p_attr[1] << 8);
                    rslt = app_bt_update_cccd(peer_cccd_data[bondindex], bondindex);
                    UNUSED_VARIABLE(rslt);
                    break;

                case HDLC_RC_CONTROLLER_GAME_EVENT_VALUE:
                    if (len != MAX_LEN_RC_CONTROLLER_GAME_EVENT)
                    {
                        return WICED_BT_GATT_INVALID_ATTR_LEN;
                    }

                    const car_event_t race_event = p_attr[0];
                    switch (race_event)
                    {
                        case CAR_EVENT_RACE_START:
                            // Reset lap count on start
                            app_rc_controller_lap[0] = 0;
                            // Enables sensors/motors, items, and lap counts
                            race_state = RACE_STATE_ACTIVE;
                            break;
                        case CAR_EVENT_RACE_RESUME:
                            // Set lap count to 1 on resume (e.g. if MCU power cycled) so that future laps are counted
                            app_rc_controller_lap[0] = 1;
                            // Re-enables sensors/motors, items, and lap counts
                            race_state = RACE_STATE_ACTIVE;
                            break;
                        case CAR_EVENT_RACE_END:
                            race_state = RACE_STATE_INACTIVE;
                            break;
                        default:
                            break;
                    }

                    break;

                /* By writing into Characteristic Client Configuration descriptor
                 * peer can enable or disable notification or indication */
                case HDLD_RC_CONTROLLER_LAP_CLIENT_CHAR_CONFIG:
                    if (len != 2)
                    {
                        return WICED_BT_GATT_INVALID_ATTR_LEN;
                    }
                    app_rc_controller_lap_client_char_config[0] = p_attr[0];
                    peer_cccd_data[bondindex] = p_attr[0] | (p_attr[1] << 8);
                    rslt = app_bt_update_cccd(peer_cccd_data[bondindex], bondindex);
                    UNUSED_VARIABLE(rslt);
                    break;

                case HDLD_GATT_SERVICE_CHANGED_CLIENT_CHAR_CONFIG:
                    gatt_status = WICED_BT_GATT_SUCCESS;
                    break;

                default:
                    gatt_status = WICED_BT_GATT_INVALID_HANDLE;
                    break;
                }
            }
            else
            {
                /* Value to write does not meet size constraints */
                gatt_status = WICED_BT_GATT_INVALID_ATTR_LEN;
            }
            break;
        }
    }
    if (!isHandleInTable)
    {
        /* Add code to read value for handles not contained within
         * generated lookup table. This is a custom logic that depends on the
         * application, and is not used in the current application. If the value
         * for the current handle is successfully written in the below code
         * snippet, then set the result using: res = WICED_BT_GATT_SUCCESS; */
        switch(attr_handle)
        {
            default:
                /* The write operation was not performed for the
                 * indicated handle */
                gatt_status = WICED_BT_GATT_WRITE_NOT_PERMIT;
                break;
        }
    }

    return gatt_status;
}

/**
 * Function Name: app_bt_send_message
 *
 * Function Description:
 *   @brief Check if client has registered for notification/indication and send
 *   message if appropriate
 *
 *   @param uint16_t
 *   The attribute handle of the message to send
 *
 *   @return None
 *
 */
void app_bt_send_message(uint16_t handle)
{
    wiced_bt_gatt_status_t status;

    UNUSED_VARIABLE(status);

    if((handle == HDLC_RC_CONTROLLER_GET_ITEM_VALUE) && (app_rc_controller_get_item_client_char_config[0] & GATT_CLIENT_CONFIG_NOTIFICATION))
    {
        status = wiced_bt_gatt_server_send_notification(ble_state.conn_id,
                                                        HDLC_RC_CONTROLLER_GET_ITEM_VALUE,
                                                        app_rc_controller_get_item_len,
                                                        app_rc_controller_get_item,
                                                        NULL);
    }
    else if((handle == HDLC_RC_CONTROLLER_LAP_VALUE) && (app_rc_controller_lap_client_char_config[0] & GATT_CLIENT_CONFIG_NOTIFICATION))
    {
        status = wiced_bt_gatt_server_send_notification(ble_state.conn_id,
                                                        HDLC_RC_CONTROLLER_LAP_VALUE,
                                                        app_rc_controller_lap_len,
                                                        app_rc_controller_lap,
                                                        NULL);
    }
}

/**
 * Function Name: app_bt_free_buffer
 *
 * Function Description:
 *   @brief This function frees up the memory buffer
 *
 *   @param uint8_t *p_data: Pointer to the buffer to be free
 *
 *   @return None
 */
void app_bt_free_buffer(uint8_t *p_buf)
{
    vPortFree(p_buf);
}

/**
 * Function Name: app_bt_alloc_buffer
 *
 * Function Description:
 *   @brief This function allocates a memory buffer.
 *
 *   @param int len: Length to allocate
 *
 *   @return None
 */
void* app_bt_alloc_buffer(int len)
{
    return pvPortMalloc(len);
}

/**
 * Function Name: app_bt_gatt_increment_notify_value
 *
 * Function Description:
 *   @brief Keep number of the button pushes in the last byte of the Hello
 *   message.That will guarantee that if client reads it, it will have correct
 *   data.
 *
 *   @param None
 *
 *   @return None
 *
 */
void app_bt_gatt_increment_notify_value(void)
{}
