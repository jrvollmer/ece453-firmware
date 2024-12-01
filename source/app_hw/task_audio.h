/**
 * @file task_audio.h
 * @author James Vollmer (jrvollmer@wisc.edu) - Team 01
 * @brief
 * Header file for CLI for controlling speaker/aduio interface
 *
 * @version 0.1
 * @date 2024-11-15
 *
 * @copyright Copyright (c) 2024
 */
#ifndef __TASK_AUDIO_H__
#define __TASK_AUDIO_H__

/* Include Infineon BSP Libraries */
#include "cy_pdl.h"
#include "cyhal.h"
#include "cybsp.h"

/* Include Standard C Libraries*/
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>

/* FreeRTOS Includes */
#include <FreeRTOS.h>
#include <event_groups.h>
#include <queue.h>
#include <semphr.h>
#include <task.h>
#include "source/FreeRTOSConfig.h"
#include "source/FreeRTOS_CLI.h"

/* Include Project Specific Files */
#include "task_console.h"


// Audio command type
typedef enum
{
    AUDIO_PLAY_TONE  = 0,
    AUDIO_PLAY_SOUND_EFFECT  = 1
} audio_cmd_type_t;

// Audio information to pass between CLI handler and audio task
typedef struct
{
    audio_cmd_type_t cmd;
    uint32_t frequency;  // Frequency (Hz)
    uint32_t amplitude;  // Amplitude (mdB)
    uint32_t sound_effect_idx;
    QueueHandle_t return_queue;
} audio_packet_t;


extern QueueHandle_t q_audio_cli_req;
extern QueueHandle_t q_audio_cli_resp;

extern TaskHandle_t xTaskAudioHandle;


void task_audio_init(void);


#endif // __TASK_AUDIO_H__
