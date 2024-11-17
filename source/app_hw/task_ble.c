/**
 * @file task_ble.c
 * @author James Vollmer (jrvollmer@wisc.edu) - Team 01
 * @brief
 * Source file for CLI for controlling BLE interface
 *
 * @version 0.1
 * @date 2024-09-28
 *
 * @copyright Copyright (c) 2024
 */
#include "task_ble.h"


/******************************************************************************
 * Private Function Declarations                                              *
 ******************************************************************************/
static void task_ble_cli(void *param);

static BaseType_t cli_handler_ble_pair(
    char *pcWriteBuffer,
    size_t xWriteBufferLen,
    const char *pcCommandString
);
static BaseType_t cli_handler_ble_check_connection(
    char *pcWriteBuffer,
    size_t xWriteBufferLen,
    const char *pcCommandString
);
static BaseType_t cli_handler_ble_notify(
    char *pcWriteBuffer,
    size_t xWriteBufferLen,
    const char *pcCommandString
);
static BaseType_t cli_handler_ble_read_joystick(
    char *pcWriteBuffer,
    size_t xWriteBufferLen,
    const char *pcCommandString
);
static BaseType_t cli_handler_ble_get_item(
    char *pcWriteBuffer,
    size_t xWriteBufferLen,
    const char *pcCommandString
);


/******************************************************************************
 * Global Variables                                                           *
 ******************************************************************************/
// Queues used to send and receive info for commands used to control the BLE interface
QueueHandle_t q_ble_cli_req;
QueueHandle_t q_ble_cli_resp;

// The CLI command definition for the BLE pair command
static const CLI_Command_Definition_t xBlePair =
{
    "pair",                           // Command text
    "\r\npair < on|off >\r\n",        // Command help text
    cli_handler_ble_pair,             // The function to run
    1                                 // The user can enter 1 parameter
};

// The CLI command definition for the BLE check connection command
static const CLI_Command_Definition_t xBleCheckConnection =
{
    "check_conn",                     // Command text
    "\r\ncheck_conn\r\n",             // Command help text
    cli_handler_ble_check_connection, // The function to run
    0                                 // The user can enter 0 parameters
};

// The CLI command definition for the BLE notify command
static const CLI_Command_Definition_t xBleNotify =
{
    "notify",                         // Command text
    "\r\nnotify < data >\r\n",        // Command help text
    cli_handler_ble_notify,           // The function to run
    1                                 // The user can enter 1 parameter
};

// The CLI command definition for the BLE read request (joystick read) command
static const CLI_Command_Definition_t xBleReadJoystick =
{
    "read_joystick",                  // Command text
    "\r\nread_joystick\r\n",          // Command help text
    cli_handler_ble_read_joystick,    // The function to run
    0                                 // The user can enter 0 parameters
};

// The CLI command definition for a BLE notify (get item) command
static const CLI_Command_Definition_t xBleGetItem =
{
    "get_item",                  // Command text
    "\r\nget_item\r\n",          // Command help text
    cli_handler_ble_get_item,    // The function to run
    0                            // The user can enter 0 parameters
};


/******************************************************************************
 * Static Function Definitions                                                *
 ******************************************************************************/
/**
 * @brief  This task receives commands from the BLE CLI message queue
 *         to execute BLE actions
 * 
 * @param param
 * Unused
 */
static void task_ble_cli(void *param)
{
    ble_cli_packet_t ble_packet;

    // Suppress warning for unused parameter
    (void)param;

    // Repeatedly running part of the task
    for (;;)
    {
        // Check the Queue. If nothing was in the queue, we should return pdFALSE
        xQueueReceive(q_ble_cli_req, &ble_packet, portMAX_DELAY);

        if (ble_packet.action == BLE_ACTION_PAIR)
        {
            wiced_result_t result;

            task_print_info("Entering pairing mode...");

            // Temporarily stop advertising and clear address resolution list
            // so that we can connect to a new device
            result = wiced_bt_start_advertisements(BTM_BLE_ADVERT_OFF,
                                                    0,
                                                    NULL);
            result = wiced_bt_ble_address_resolution_list_clear_and_disable();
            if (WICED_BT_SUCCESS != result)
            {
                task_print_warning("Failed to clear address resolution list");
            }

            // Begin advertising so that clients can pair with us
            result = wiced_bt_start_advertisements(BTM_BLE_ADVERT_UNDIRECTED_HIGH,
                                                    0,
                                                    NULL);
            if (result == WICED_BT_SUCCESS)
            {
                // Indicate to other areas of the BLE stack that we are in
                // pairing mode
                pairing_mode = TRUE;
            }
            else
            {
                task_print_warning("Failed to start advertising: %d", result);
            }

            // Send packet back once done
            xQueueSend(ble_packet.return_queue, &ble_packet, portMAX_DELAY);
        }
        else if (ble_packet.action == BLE_ACTION_UNPAIR)
        {
            wiced_result_t result;

            task_print_info("Exiting pairing mode...");

            result = wiced_bt_start_advertisements(BTM_BLE_ADVERT_OFF,
                                                    0,
                                                    NULL);
            if (result == WICED_BT_SUCCESS)
            {
                // Indicate to other areas of the BLE stack that we are no longer
                // in pairing mode
                pairing_mode = FALSE;
            }
            else
            {
                task_print_info("Failed to stop advertising: %d", result);
            }

            // Send packet back once done
            xQueueSend(ble_packet.return_queue, &ble_packet, portMAX_DELAY);
        }
        else if (ble_packet.action == BLE_ACTION_CHECK_CONN)
        {
            if (hello_sensor_state.conn_id)
            {
                task_print_info("Connected with connection ID '%d'", hello_sensor_state.conn_id);
            }
            else
            {
                task_print_info("Not connected to any clients");
            }

            // Send packet back once done
            xQueueSend(ble_packet.return_queue, &ble_packet, portMAX_DELAY);
        }
        else if (ble_packet.action == BLE_ACTION_NOTIFY)
        {
            app_rc_controller_get_item[0] = ble_packet.data;

            // Send if client is registered to receive notifications
            app_bt_send_message();

            // Tell user if notification was sent or not
            if(app_rc_controller_get_item_client_char_config[0] & GATT_CLIENT_CONFIG_NOTIFICATION)
            {
                task_print_info("Sent item value 0x%0x to client '%d'", ble_packet.data, hello_sensor_state.conn_id);
            }
            else
            {
                task_print_warning("Client is not registered to receive notifications");
                task_print_info("Client '%d' can still use a read request to get item value 0x%0x", hello_sensor_state.conn_id, ble_packet.data);
            }

            // Send packet back once done
            xQueueSend(ble_packet.return_queue, &ble_packet, portMAX_DELAY);
        }
        else if (ble_packet.action == BLE_ACTION_READ_JOYSTICK)
        {
            car_joystick_t joystick_val;
            uint32_t joystick_hex;
            car_item_t use_item_val;
            const car_item_t zero = 0;

            // Get the joystick x and y values from the client (hex is for demo purposes)
            xQueueReceive(q_ble_car_joystick_y, &joystick_val, 0);
            memcpy(&joystick_hex, &joystick_val, sizeof(car_joystick_t));
            // TODO REMOVE
            // memcpy(&joystick_val, &app_rc_controller_joystick_y, app_rc_controller_joystick_y_len);
            // memcpy(&joystick_hex, &app_rc_controller_joystick_y, app_rc_controller_joystick_y_len);
            task_print_info("Joystick Y value is %f (0x%0x)", joystick_val, joystick_hex);
            xQueueReceive(q_ble_car_joystick_x, &joystick_val, 0);
            memcpy(&joystick_hex, &joystick_val, sizeof(car_joystick_t));
            // TODO REMOVE
            // memcpy(&joystick_val, &app_rc_controller_joystick_x, app_rc_controller_joystick_x_len);
            // memcpy(&joystick_hex, &app_rc_controller_joystick_x, app_rc_controller_joystick_x_len);
            task_print_info("Joystick X value is %f (0x%0x)", joystick_val, joystick_hex);

            // Get and reset use item value
            memcpy(&use_item_val, &app_rc_controller_use_item, app_rc_controller_use_item_len);
            task_print_info("Use Item value is %u", use_item_val);
            memcpy(&app_rc_controller_use_item, &zero, app_rc_controller_use_item_len);

            // Send packet back once done
            xQueueSend(ble_packet.return_queue, &ble_packet, portMAX_DELAY);
        }
        else if (ble_packet.action == BLE_ACTION_GET_ITEM)
        {
            if (app_bt_car_get_new_item() == pdTRUE)
            {
                task_print_info("Sent new item to RC controller app");
            }
            else
            {
                task_print_info("Did not get new item");
            }

            // Send packet back once done
            xQueueSend(ble_packet.return_queue, &ble_packet, portMAX_DELAY);
        }
    }
}


/**
 * @brief  FreeRTOS CLI Handler for the 'pair' command
 * 
 * @param pcWriteBuffer
 * Array used to return a string to the CLI parser
 * @param xWriteBufferLen
 * The length of the write buffer
 * @param pcCommandString
 * The list of parameters entered by the user
 * @return BaseType_t
 * pdFALSE to indicate command completion
 */
static BaseType_t cli_handler_ble_pair(
    char *pcWriteBuffer,
    size_t xWriteBufferLen,
    const char *pcCommandString
)
{
    ble_cli_packet_t ble_packet;
    BaseType_t xParameterStringLength;
    const char *pcParameter;

    ble_packet.msg = BLE_PAIR;

    // Remove compile time warnings about unused parameters, and check the
    // write buffer is not NULL.
    // NOTE - for simplicity, this example assumes the write buffer length
    // is adequate, so does not check for buffer overflows
    (void)pcCommandString;
    (void)xWriteBufferLen;
    configASSERT(pcWriteBuffer);

    // Obtain the parameter string
    pcParameter = FreeRTOS_CLIGetParameter(
        pcCommandString,        // The command string itself
        1,                      // Return the 1st parameter
        &xParameterStringLength // Store the parameter string length
    );
    // Sanity check something was returned
    configASSERT(pcParameter);

    // Copy ONLY the parameter to pcWriteBuffer
    memset(pcWriteBuffer, 0x00, xWriteBufferLen);
    strncat(pcWriteBuffer, pcParameter, xParameterStringLength);

    // Validate input
    if (strncmp(pcWriteBuffer, "on", 4) == 0)
    {
        ble_packet.action = BLE_ACTION_PAIR;
    }
    else if (strncmp(pcWriteBuffer, "off", 4) == 0)
    {
        ble_packet.action = BLE_ACTION_UNPAIR;
    }
    else
    {
		// Clear the return string
		memset(pcWriteBuffer, 0x00, xWriteBufferLen);
		sprintf(pcWriteBuffer, "\n\r\tInvalid input. Specify 'on' or 'off'");
		return pdFALSE;
    }

    // Send the message to the BLE task
    ble_packet.return_queue = q_ble_cli_resp;
    xQueueSendToBack(q_ble_cli_req, &ble_packet, portMAX_DELAY);

    // Wait for pairing to complete
    xQueueReceive(q_ble_cli_resp, &ble_packet, portMAX_DELAY);

    // Nothing to return, so zero out the pcWriteBuffer
    memset(pcWriteBuffer, 0, xWriteBufferLen);

    return pdFALSE;
}

/**
 * @brief  FreeRTOS CLI Handler for the 'check_conn' command
 * 
 * @param pcWriteBuffer
 * Array used to return a string to the CLI parser
 * @param xWriteBufferLen
 * The length of the write buffer
 * @param pcCommandString
 * The list of parameters entered by the user
 * @return BaseType_t
 * pdFALSE to indicate command completion
 */
static BaseType_t cli_handler_ble_check_connection(
    char *pcWriteBuffer,
    size_t xWriteBufferLen,
    const char *pcCommandString
)
{
    ble_cli_packet_t ble_packet;

    ble_packet.action = BLE_ACTION_CHECK_CONN;

    // Remove compile time warnings about unused parameters, and check the
    // write buffer is not NULL.
    // NOTE - for simplicity, this example assumes the write buffer length
    // is adequate, so does not check for buffer overflows
    (void)pcCommandString;
    (void)xWriteBufferLen;
    configASSERT(pcWriteBuffer);

    // Send the message to the BLE task
    ble_packet.return_queue = q_ble_cli_resp;
    xQueueSendToBack(q_ble_cli_req, &ble_packet, portMAX_DELAY);

    // Wait for connection to complete
    xQueueReceive(q_ble_cli_resp, &ble_packet, portMAX_DELAY);

    // Nothing to return, so zero out the pcWriteBuffer
    memset(pcWriteBuffer, 0, xWriteBufferLen);

    return pdFALSE;
}

/**
 * @brief  FreeRTOS CLI Handler for the 'notify' command
 * 
 * @param pcWriteBuffer
 * Array used to return a string to the CLI parser
 * @param xWriteBufferLen
 * The length of the write buffer
 * @param pcCommandString
 * The list of parameters entered by the user
 * @return BaseType_t
 * pdFALSE to indicate command completion
 */
static BaseType_t cli_handler_ble_notify(
    char *pcWriteBuffer,
    size_t xWriteBufferLen,
    const char *pcCommandString
)
{
    const char *pcParameter;
    ble_cli_packet_t ble_packet;
    BaseType_t xParameterStringLength;
    BaseType_t xReturn = pdFALSE;
	char *end_ptr;

    ble_packet.action = BLE_ACTION_NOTIFY;

    // Remove compile time warnings about unused parameters, and check the
    // write buffer is not NULL.
    // NOTE - for simplicity, this example assumes the write buffer length
    // is adequate, so does not check for buffer overflows
    (void)pcCommandString;
    (void)xWriteBufferLen;
    configASSERT(pcWriteBuffer);

    // Obtain the parameter string
    pcParameter = FreeRTOS_CLIGetParameter(
        pcCommandString,        // The command string itself
        1,                      // Return the 1st parameter
        &xParameterStringLength // Store the parameter string length
    );
    // Sanity check something was returned
    configASSERT(pcParameter);

    // Copy ONLY the parameter to pcWriteBuffer
    memset(pcWriteBuffer, 0x00, xWriteBufferLen);
    strncat(pcWriteBuffer, pcParameter, xParameterStringLength);

	// Convert the string to a number
    long raw_val = strtol(pcWriteBuffer, &end_ptr, 16);
	ble_packet.data = raw_val & 0xFF;
	if ((*end_ptr != '\0') || (ble_packet.data != raw_val))
	{
		// Clear the return string
		memset(pcWriteBuffer, 0x00, xWriteBufferLen);
		sprintf(pcWriteBuffer, "\n\r\tInvalid data: %lu. Must be one hex byte", raw_val);
		return xReturn;
	}

    // Send the message to the BLE task
    ble_packet.return_queue = q_ble_cli_resp;
    xQueueSendToBack(q_ble_cli_req, &ble_packet, portMAX_DELAY);

    // Wait for send to complete
    xQueueReceive(q_ble_cli_resp, &ble_packet, portMAX_DELAY);

    // Nothing to return, so zero out the pcWriteBuffer
    memset(pcWriteBuffer, 0, xWriteBufferLen);

    return xReturn;
}

/**
 * @brief  FreeRTOS CLI Handler for the 'read_joystick' command.
 *         The "joystick" value is a mock value set by the client
 *         to mimic an RC controller joystick value.
 * 
 * @param pcWriteBuffer
 * Array used to return a string to the CLI parser
 * @param xWriteBufferLen
 * The length of the write buffer
 * @param pcCommandString
 * The list of parameters entered by the user
 * @return BaseType_t
 * pdFALSE to indicate command completion
 */
static BaseType_t cli_handler_ble_read_joystick(
    char *pcWriteBuffer,
    size_t xWriteBufferLen,
    const char *pcCommandString
)
{
    ble_cli_packet_t ble_packet;
    BaseType_t xReturn = pdFALSE;

    ble_packet.action = BLE_ACTION_READ_JOYSTICK;

    // Remove compile time warnings about unused parameters, and check the
    // write buffer is not NULL.
    // NOTE - for simplicity, this example assumes the write buffer length
    // is adequate, so does not check for buffer overflows
    (void)pcCommandString;
    (void)xWriteBufferLen;
    configASSERT(pcWriteBuffer);

    // Send the message to the BLE task
    ble_packet.return_queue = q_ble_cli_resp;
    xQueueSendToBack(q_ble_cli_req, &ble_packet, portMAX_DELAY);

    // Wait for send to complete
    xQueueReceive(q_ble_cli_resp, &ble_packet, portMAX_DELAY);

    // Nothing to return, so zero out the pcWriteBuffer
    memset(pcWriteBuffer, 0, xWriteBufferLen);

    return xReturn;
}

/**
 * @brief  FreeRTOS CLI Handler for the 'get_item' command
 * 
 * @param pcWriteBuffer
 * Array used to return a string to the CLI parser
 * @param xWriteBufferLen
 * The length of the write buffer
 * @param pcCommandString
 * The list of parameters entered by the user
 * @return BaseType_t
 * pdFALSE to indicate command completion
 */
static BaseType_t cli_handler_ble_get_item(
    char *pcWriteBuffer,
    size_t xWriteBufferLen,
    const char *pcCommandString
)
{
    ble_cli_packet_t ble_packet;

    ble_packet.action = BLE_ACTION_GET_ITEM;

    // Remove compile time warnings about unused parameters, and check the
    // write buffer is not NULL.
    // NOTE - for simplicity, this example assumes the write buffer length
    // is adequate, so does not check for buffer overflows
    (void)pcCommandString;
    (void)xWriteBufferLen;
    configASSERT(pcWriteBuffer);

    // Send the message to the BLE task
    ble_packet.return_queue = q_ble_cli_resp;
    xQueueSendToBack(q_ble_cli_req, &ble_packet, portMAX_DELAY);

    // Wait for connection to complete
    xQueueReceive(q_ble_cli_resp, &ble_packet, portMAX_DELAY);

    // Nothing to return, so zero out the pcWriteBuffer
    memset(pcWriteBuffer, 0, xWriteBufferLen);

    return pdFALSE;
}

/******************************************************************************
 * Public Function Definitions                                                *
 ******************************************************************************/
void task_ble_init(void)
{
    // Create the Queues used to control BLE from CLI
    q_ble_cli_req  = xQueueCreate(1, sizeof(ble_cli_packet_t));
    q_ble_cli_resp = xQueueCreate(1, sizeof(ble_cli_packet_t));

    // Register the CLI commands
    FreeRTOS_CLIRegisterCommand(&xBlePair);
    FreeRTOS_CLIRegisterCommand(&xBleCheckConnection);
    FreeRTOS_CLIRegisterCommand(&xBleNotify);
    FreeRTOS_CLIRegisterCommand(&xBleReadJoystick);
    FreeRTOS_CLIRegisterCommand(&xBleGetItem);

    // Create the task that will control BLE via the CLI
    xTaskCreate(
        task_ble_cli,
        "Task_BLE_CLI",
        configMINIMAL_STACK_SIZE * 2,
        NULL,
        configMAX_PRIORITIES - 6,
        NULL
    );
}
