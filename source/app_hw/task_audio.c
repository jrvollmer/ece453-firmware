/**
 * @file task_audio.c
 * @author James Vollmer (jrvollmer@wisc.edu) - Team 01
 * @brief
 * Source file for CLI for controlling speaker/audio interface
 *
 * @version 0.1
 * @date 2024-11-15
 *
 * @copyright Copyright (c) 2024
 */
#include "task_audio.h"
#include "app_bt_car.h"
#include "app_audio.h"
#include "cy_pdl.h"
#include "cybsp.h"


/******************************************************************************
 * Private Function Declarations                                              *
 ******************************************************************************/
static void task_audio_car(void *param);
static void task_audio_cli(void *param);

static BaseType_t cli_handler_audio_play_tone(
    char *pcWriteBuffer,
    size_t xWriteBufferLen,
    const char *pcCommandString
);
static BaseType_t cli_handler_audio_play_tune(
    char *pcWriteBuffer,
    size_t xWriteBufferLen,
    const char *pcCommandString
);


/******************************************************************************
 * Global Variables                                                           *
 ******************************************************************************/
// Queues used to send and receive info for commands used to control the audio interface
QueueHandle_t q_audio_cli_req;
QueueHandle_t q_audio_cli_resp;

TaskHandle_t xTaskAudioHandle;

// The CLI command definition for the audio play tone command
static const CLI_Command_Definition_t xAudioPlayTone =
{
    "play_tone",                                           // Command text
    "\r\nplay_tone < frequency > < amplitude >\r\n\t200 <= freq (Hz) <= 5000\r\n\t0 <= amp (mdB) <= 94000\r\n",  // Command help text
    cli_handler_audio_play_tone,                           // The function to run
    2                                                      // The user can enter 2 parameters
};

// The CLI command definition for the audio play tune command
static const CLI_Command_Definition_t xAudioPlayTune =
{
    "play_tune",                       // Command text
    "\r\nplay_tune\r\n",               // Command help text
    cli_handler_audio_play_tune,       // The function to run
    0                                  // The user can enter 0 parameters
};


/******************************************************************************
 * Static Function Definitions                                                *
 ******************************************************************************/
/**
 * @brief  Play tunes when notified
 * 
 * @param param
 * Unused
 */
static void task_audio_car(void *param)
{
    // Suppress warning for unused parameter
    (void)param;

    // Repeatedly running part of the task
    for (;;)
    {
        // Wait to be notified, getting the item to use
        const car_item_t item = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        task_print_info("Playing tune"); // TODO REMOVE
        app_audio_play_tune(); // TODO specify item
    }
}


/**
 * @brief  This task receives commands from the audio message queue to execute
 *         audio actions
 * 
 * @param param
 * Unused
 */
static void task_audio_cli(void *param)
{
    audio_packet_t audio_pkt;

    // Suppress warning for unused parameter
    (void)param;

    // Repeatedly running part of the task
    for (;;)
    {
        // Check the Queue. If nothing was in the queue, we should return pdFALSE
        xQueueReceive(q_audio_cli_req, &audio_pkt, portMAX_DELAY);

        switch(audio_pkt.cmd) {
            case AUDIO_PLAY_TONE:
                task_print_info("Playing tone with frequency %u Hz and amplitude %u mdB...", audio_pkt.frequency, audio_pkt.amplitude);
                app_audio_play_tone(audio_pkt.amplitude, audio_pkt.frequency);
                break;

            case AUDIO_PLAY_TUNE:
                task_print_info("Playing tune");
                app_audio_play_tune();
                break;

            default:
                // Only commands above are supported
                break;
        }

        // Send packet back to CLI handler once action is complete
        xQueueSend(audio_pkt.return_queue, &audio_pkt, portMAX_DELAY);
    }
}


/**
 * @brief
 * This function parses the list of parameters for a uint at the provided index.
 * 'arg' will be set to the parsed value
 *
 * @param pcWriteBuffer
 * Array used to return a string to the CLI parser
 * @param xWriteBufferLen
 * The length of the write buffer
 * @param pcCommandString
 * The list of parameters entered by the user
 * @param argname
 * A name for identifying the parameter in the event of invalid user input
 * @param argidx
 * Index of the CLA to be parsed (1-indexed)
 * @param argmin
 * Minimum allowable value for arg
 * @param argmax
 * Maximum allowable value for arg
 * @param arg
 * A pointer to the uint32_t that will be set according to the parameter, if valid
 * @return BaseType_t
 * pdTRUE  if a valid uint is parsed.
 * pdFALSE if an invalid value is parsed.
 */
static BaseType_t cli_handler_audio_get_and_check_uint_arg(
    char *pcWriteBuffer,
    size_t xWriteBufferLen,
    const char *pcCommandString,
    const char *argname,
    const uint8_t argidx,
    const uint32_t argmin,
    const uint32_t argmax,
    uint32_t *arg
)
{
    const char *pcParameter;
    BaseType_t xParameterStringLength;
    BaseType_t xReturn = pdFALSE;
	char *end_ptr;
    long raw_long_arg;

    // Obtain the parameter string
    pcParameter = FreeRTOS_CLIGetParameter(
        pcCommandString,        // The command string itself
        argidx,                 // Return the <argidx>st parameter
        &xParameterStringLength // Store the parameter string length
    );
    // Sanity check something was returned
    configASSERT(pcParameter);

    // Copy ONLY the parameter to pcWriteBuffer
    memset(pcWriteBuffer, 0x00, xWriteBufferLen);
    strncat(pcWriteBuffer, pcParameter, xParameterStringLength);

	// Convert the string to a number and check bounds
	raw_long_arg = strtol(pcWriteBuffer, &end_ptr, 10);
	if ((*end_ptr != '\0') || (raw_long_arg < argmin) || (raw_long_arg > argmax))
	{
		// Clear the return string
		memset(pcWriteBuffer, 0x00, xWriteBufferLen);
		sprintf(pcWriteBuffer, "\n\r\tInvalid %s value, %li. Must be between %lu and %lu", argname, raw_long_arg, argmin, argmax);
	}
    else
    {
        *arg = (uint32_t)raw_long_arg;
        xReturn = pdTRUE;
    }

    return xReturn;
}

/**
 * @brief  FreeRTOS CLI Handler for the 'play_tone' command
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
static BaseType_t cli_handler_audio_play_tone(
    char *pcWriteBuffer,
    size_t xWriteBufferLen,
    const char *pcCommandString
)
{
    audio_packet_t audio_pkt;
    BaseType_t xReturn;

    audio_pkt.cmd = AUDIO_PLAY_TONE;

    // Remove compile time warnings about unused parameters, and check the
    // write buffer is not NULL.
    // NOTE - for simplicity, this example assumes the write buffer length
    // is adequate, so does not check for buffer overflows
    (void)pcCommandString;
    (void)xWriteBufferLen;
    configASSERT(pcWriteBuffer);

    // Get frequency and amplitude, with validation
    xReturn = cli_handler_audio_get_and_check_uint_arg(
        pcWriteBuffer,
        xWriteBufferLen,
        pcCommandString,
        "frequency",
        1,
        MIN_FREQ_HZ,
        MAX_FREQ_HZ,
        &audio_pkt.frequency
    );
    if (xReturn == pdFALSE)
    {
        return xReturn;
    }
    xReturn = cli_handler_audio_get_and_check_uint_arg(
        pcWriteBuffer,
        xWriteBufferLen,
        pcCommandString,
        "amplitude",
        2,
        MIN_AMP_mDB,
        MAX_AMP_mDB,
        &audio_pkt.amplitude
    );
    if (xReturn == pdFALSE)
    {
        return xReturn;
    }

    // Send the message to the audio task
    audio_pkt.return_queue = q_audio_cli_resp;
    xQueueSendToBack(q_audio_cli_req, &audio_pkt, portMAX_DELAY);

    // Wait for task to complete
    xQueueReceive(q_audio_cli_resp, &audio_pkt, portMAX_DELAY);

    // Nothing to return, so zero out the pcWriteBuffer
    memset(pcWriteBuffer, 0, xWriteBufferLen);

    xReturn = pdFALSE;
    return xReturn;
}

/**
 * @brief  FreeRTOS CLI Handler for the 'play_tune' command
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
static BaseType_t cli_handler_audio_play_tune(
    char *pcWriteBuffer,
    size_t xWriteBufferLen,
    const char *pcCommandString
)
{
    audio_packet_t audio_pkt = {0};
    BaseType_t xReturn;

    audio_pkt.cmd = AUDIO_PLAY_TUNE;

    // Remove compile time warnings about unused parameters, and check the
    // write buffer is not NULL.
    // NOTE - for simplicity, this example assumes the write buffer length
    // is adequate, so does not check for buffer overflows
    (void)pcCommandString;
    (void)xWriteBufferLen;
    configASSERT(pcWriteBuffer);

    // Send the message to the audio task
    audio_pkt.return_queue = q_audio_cli_resp;
    xQueueSendToBack(q_audio_cli_req, &audio_pkt, portMAX_DELAY);

    // Wait for task to complete
    xQueueReceive(q_audio_cli_resp, &audio_pkt, portMAX_DELAY);

    // Nothing to return, so zero out the pcWriteBuffer
    memset(pcWriteBuffer, 0, xWriteBufferLen);

    xReturn = pdFALSE;
    return xReturn;
}


/******************************************************************************
 * Public Function Definitions                                                *
 ******************************************************************************/
void task_audio_init(void)
{
    // Create the Queues used to control the speaker/audio interface
    q_audio_cli_req  = xQueueCreate(1, sizeof(audio_packet_t));
    q_audio_cli_resp = xQueueCreate(1, sizeof(audio_packet_t));

    // Register the CLI commands
    FreeRTOS_CLIRegisterCommand(&xAudioPlayTone);
    FreeRTOS_CLIRegisterCommand(&xAudioPlayTune);

    // Create the task that will control audio via CLI
    xTaskCreate(
        task_audio_cli,
        "Task_Audio_CLI",
        configMINIMAL_STACK_SIZE * 2,
        NULL,
        configMAX_PRIORITIES - 6,
        NULL
    );

    // Create the task that will control audio via car interfaces
    xTaskCreate(
        task_audio_car,
        "Task_Audio_Car",
        configMINIMAL_STACK_SIZE, // TODO Verify enough
        NULL,
        configMAX_PRIORITIES - 5,
        &xTaskAudioHandle
    );
}
