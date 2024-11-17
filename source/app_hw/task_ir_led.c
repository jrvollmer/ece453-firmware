/**
 * @file task_ir_led.c
 * @author James Vollmer (jrvollmer@wisc.edu) - Team 01
 * @brief
 * Source file for CLI for controlling IR LED
 *
 * @version 0.1
 * @date 2024-11-15
 *
 * @copyright Copyright (c) 2024
 */
#include "task_ir_led.h"
#include "app_bt_car.h"
#include "app_ir_led.h"
#include "app_audio.h"


/******************************************************************************
 * Private Function Declarations                                              *
 ******************************************************************************/
static void task_ir_led_car(void *param);
static void task_ir_led_cli(void *param);

static BaseType_t cli_handler_ir_led_set_state(
    char *pcWriteBuffer,
    size_t xWriteBufferLen,
    const char *pcCommandString
);


/******************************************************************************
 * Global Variables                                                           *
 ******************************************************************************/
// Queues used to send and receive info for commands used to control the IR LED
QueueHandle_t q_ir_led_cli_req;
QueueHandle_t q_ir_led_cli_resp;

TaskHandle_t xTaskIrLedHandle;

// The CLI command definition for the IR LED set state command
static const CLI_Command_Definition_t xIrLedSetState=
{
    "set_ir_state",                     // Command text
    "\r\nset_ir_state < on|off >\r\n",  // Command help text
    cli_handler_ir_led_set_state,       // The function to run
    1                                   // The user can enter 1 parameter
};


/******************************************************************************
 * Static Function Definitions                                                *
 ******************************************************************************/
/**
 * @brief  Task for executing IR LED actions when notified
 * 
 * @param param
 * Unused
 */
static void task_ir_led_car(void *param)
{
    // Suppress warning for unused parameter
    (void)param;

    // Repeatedly running part of the task
    for (;;)
    {
        // Wait to be notified, getting the item to use
        const car_item_t item = (car_item_t)ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        BaseType_t used = app_ir_led_use_item(item); // TODO Won't use return value in the end

        if (used == pdTRUE)
        {
            task_print_info("Using item %u...", item); // TODO REMOVE
        }
        else
        {
            task_print_info("Gave an invalid item: %u", item); // TODO REMOVE
        }
    }
}


/**
 * @brief  This task receives commands from the IR LED message queue to interact
 *         with the IR LED
 * 
 * @param param
 * Unused
 */
static void task_ir_led_cli(void *param)
{
    ir_led_packet_t ir_led_pkt;

    // Suppress warning for unused parameter
    (void)param;

    // Repeatedly running part of the task
    for (;;)
    {
        // Check the Queue. If nothing was in the queue, we should return pdFALSE
        xQueueReceive(q_ir_led_cli_req, &ir_led_pkt, portMAX_DELAY);

        switch(ir_led_pkt.cmd) {
            case IR_LED_SET_STATE:
                task_print_info("Turning IR LED %s...", ir_led_pkt.state ? "on" : "off");

                if (ir_led_pkt.state)
                {
                    // Pulse the IR LED at 38kHz
                    Cy_TCPWM_TriggerStart(TCPWM0, tcpwm_0_cnt_5_MASK);
                }
                else
                {
                    // Turn the IR LED off
                    Cy_TCPWM_TriggerStopOrKill(TCPWM0, tcpwm_0_cnt_5_MASK);
                }
                break;

            default:
                // Only commands above are supported
                break;
        }

        // Send packet back to CLI handler once action is complete
        xQueueSend(ir_led_pkt.return_queue, &ir_led_pkt, portMAX_DELAY);
    }
}


/**
 * @brief  FreeRTOS CLI Handler for the 'set_state' command
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
static BaseType_t cli_handler_ir_led_set_state(
    char *pcWriteBuffer,
    size_t xWriteBufferLen,
    const char *pcCommandString
)
{
    ir_led_packet_t ir_led_pkt;
    BaseType_t xReturn = pdFALSE;
    const char *pcParameter;
    BaseType_t xParameterStringLength;

    ir_led_pkt.cmd = IR_LED_SET_STATE;

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

	// Set desired state, checking that the argument is valid
    bool set_on = !strncmp(pcParameter, "on", 4);
    bool set_off = !strncmp(pcParameter, "off", 4);
	if (!(set_on || set_off))
	{
		// Clear the return string
		memset(pcWriteBuffer, 0x00, xWriteBufferLen);
		sprintf(pcWriteBuffer, "\n\r\tInvalid state, %s. Must be either 'on' or 'off'", pcParameter);
    
        return xReturn;
	}
    ir_led_pkt.state = set_on;

    // Send the message to the IR LED task
    ir_led_pkt.return_queue = q_ir_led_cli_resp;
    xQueueSendToBack(q_ir_led_cli_req, &ir_led_pkt, portMAX_DELAY);

    // Wait for pairing to complete
    xQueueReceive(q_ir_led_cli_resp, &ir_led_pkt, portMAX_DELAY);

    // Nothing to return, so zero out the pcWriteBuffer
    memset(pcWriteBuffer, 0, xWriteBufferLen);

    return xReturn;
}


/******************************************************************************
 * Public Function Definitions                                                *
 ******************************************************************************/
void task_ir_led_init(void)
{
    // Create the Queues used to control the IR LED
    q_ir_led_cli_req  = xQueueCreate(1, sizeof(ir_led_packet_t));
    q_ir_led_cli_resp = xQueueCreate(1, sizeof(ir_led_packet_t));

    // Register the CLI commands
    FreeRTOS_CLIRegisterCommand(&xIrLedSetState);

    // Create the task that will control the IR LED via CLI
    xTaskCreate(
        task_ir_led_cli,
        "Task_IR_LED_CLI",
        configMINIMAL_STACK_SIZE * 2,
        NULL,
        configMAX_PRIORITIES - 6,
        NULL
    );

    // Create the task that will control the IR LED via RC controller
    xTaskCreate(
        task_ir_led_car,
        "Task_IR_LED_Car",
        configMINIMAL_STACK_SIZE, // TODO Verify it's enough
        NULL,
        configMAX_PRIORITIES - 5,
        &xTaskIrLedHandle
    );
}
