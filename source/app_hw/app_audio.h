/**
 * @file app_audio.h
 * @author James Vollmer (jrvollmer@wisc.edu) - Team 01
 * @brief
 * Header file for audio system
 * 
 * @version 0.1
 * @date 2024-11-15
 * 
 * @copyright Copyright (c) 2024
 */

#ifndef __APP_AUDIO_H__
#define __APP_AUDIO_H__

// Include Standard C Libraries
#include <stdint.h>
#include <stdlib.h>


// Defines
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
#define MAX_DAC_SAMPLE       (4095UL)
#define N_TONE_SAMPLES       (100)
#define N_TUNE_SAMPLES       (2260) // From MP3 processing script

#define DMA_DESCRIPTOR_MAX_SIZE                (256)
#define DMA_DESCRIPTOR_SRC_OFFSET(T,BASE,I)    ((T*) ((BASE) + ((DMA_DESCRIPTOR_MAX_SIZE) * (I))))


// Function declarations
void app_audio_init(void);
void app_audio_play_tone(uint32_t amplitude, uint32_t frequency);
void app_audio_play_tune(void);


#endif // __APP_AUDIO_H__
