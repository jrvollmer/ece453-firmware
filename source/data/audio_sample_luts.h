/**
 * @file audio_sample_luts.h
 * @author James Vollmer (jrvollmer@wisc.edu) - Team 01
 * @brief
 * Header file with audio sample LUTs for sound effects
 * 
 * @version 0.1
 * @date 2024-12-1
 * 
 * @copyright Copyright (c) 2024
 */

#ifndef __AUDIO_SAMPLE_LUTS_H__
#define __AUDIO_SAMPLE_LUTS_H__

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
// Standard C libraries
#include <stdint.h>
#include <stdlib.h>


/******************************************************************************/
/* Defines and Typedefs                                                       */
/******************************************************************************/
// Sample rates
// TODO See if we can increase the sample rate without harming the speaker as long as the highest frequency component isn't above 20kHz
#define AUDIO_SOUND_EFFECT_SAMPLE_RATE_HZ_GET_ITEM      (20000UL) // TODO Under-sampled (original sample rate is 44100)
#define AUDIO_SOUND_EFFECT_SAMPLE_RATE_HZ_USE_SHIELD    (20000UL) // TODO Slightly under-sampled (original sample rate is 24000)
#define AUDIO_SOUND_EFFECT_SAMPLE_RATE_HZ_USE_SHOT      (20000UL) // TODO Under-sampled (original sample rate is 44100)
#define AUDIO_SOUND_EFFECT_SAMPLE_RATE_HZ_BOOST         (20000UL) // TODO Slightly under-sampled (original sample rate is 24000)
#define AUDIO_SOUND_EFFECT_SAMPLE_RATE_HZ_HIT           (20000UL) // TODO Under-sampled (original sample rate is 44100)
// Sample sizes
#define AUDIO_SOUND_EFFECT_SAMPLE_SIZE_GET_ITEM      (21036U)
#define AUDIO_SOUND_EFFECT_SAMPLE_SIZE_USE_SHIELD    (16355U)
#define AUDIO_SOUND_EFFECT_SAMPLE_SIZE_USE_SHOT      (14624U)
#define AUDIO_SOUND_EFFECT_SAMPLE_SIZE_BOOST         (32437U)
#define AUDIO_SOUND_EFFECT_SAMPLE_SIZE_HIT           (19726U)

typedef enum
{
    AUDIO_SOUND_EFFECT_GET_ITEM   = 0,
    AUDIO_SOUND_EFFECT_USE_SHIELD = 1,
    AUDIO_SOUND_EFFECT_USE_SHOT   = 2,
    AUDIO_SOUND_EFFECT_BOOST      = 3,
    AUDIO_SOUND_EFFECT_HIT        = 4,
    AUDIO_SOUND_EFFECT_MAX        = 5
} audio_sound_effect_e;


/******************************************************************************/
/* Extern Data Declarations                                                   */
/******************************************************************************/
// Number of samples for sound effects
extern const uint16_t audio_sample_sizes[AUDIO_SOUND_EFFECT_MAX];
// Sample rates for sound effects
extern const uint32_t audio_sample_rates[AUDIO_SOUND_EFFECT_MAX];
// VDAC sample LUTs for sound effects
extern const uint32_t* audio_sample_luts[AUDIO_SOUND_EFFECT_MAX];


#endif // __AUDIO_SAMPLE_LUTS_H__
