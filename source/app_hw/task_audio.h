/**
 * @file task_audio.h
 * @author James Vollmer (jrvollmer@wisc.edu) - Team 01
 * @brief
 * Header file for CLI for controlling speaker/aduio interface
 *
 * @version 0.1
 * @date 2024-10-16
 *
 * @copyright Copyright (c) 2024
 */
#ifndef __TASK_AUDIO_H__
#define __TASK_AUDIO_H__

#include "main.h"


// Defines and typedefs
// NOTE: While the speaker is capable of hitting 20kHz, the nominal max frequency for the
//       CTDAC's clock is 500kHz; Sine the since wave LUT contains 100 samples, this gives
//       a maximum frequency of 5kHz, limited by the MCU, which should be plenty for our
//       needs anyway (~1kHz higher than a piano's highest note). In our project, we can
//       reduce our sampling rate as needed to balance frequency requirements
#define MIN_FREQ_HZ          (200)
#define MAX_FREQ_HZ          (5000)
#define MIN_AMP_mDB          (0)
#define MAX_AMP_mDB          (94000) // Speaker rating
#define TUNE_NAME_MAX_LEN    (25)
#define N_TUNES              (2)
#define N_TONE_SAMPLES       (100)
#define N_TUNE_SAMPLES       (2260) // From MP3 processing script
#define MAX_DAC_SAMPLE       (4095UL)

// Audio command type
typedef enum
{
    AUDIO_PLAY_TONE  = 0,
    AUDIO_PLAY_TUNE  = 1
} audio_cmd_type_t;

// Audio information to pass between CLI handler and audio task
typedef struct
{
    audio_cmd_type_t cmd;
    uint32_t frequency;  // Frequency (Hz)
    uint32_t amplitude;  // Amplitude (mdB)
    QueueHandle_t return_queue;
} audio_packet_t;


extern QueueHandle_t q_audio_req;
extern QueueHandle_t q_audio_resp;

extern uint32_t transformedSineWaveLUT[];
extern uint32_t soundByteSamplesLUT[N_TUNE_SAMPLES];

void task_audio_init(void);


#endif // __TASK_AUDIO_H__
