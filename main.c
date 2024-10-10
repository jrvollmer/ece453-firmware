/**
 * @file main.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @author James Vollmer (jrvollmer@wisc.edu) - Team 01
 * @brief 
 * @version 0.2
 * @date 2024-10-16
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#include "main.h"
#include "cy_pdl.h"
#include "cybsp.h"
#include "source/app_hw/task_console.h"
#include "source/app_hw/task_ir_led.h"
#include "source/app_hw/task_audio.h"


/******************************************************************************/
/* Global Variables                                                           */
/******************************************************************************/


/******************************************************************************/
/* Macros                                                                     */
/******************************************************************************/


/*******************************************************************************
 * Function Prototypes
 ********************************************************************************/
static void vdac_start(void);
static void dma_start(uint32_t sineWaveLUT[], uint32_t *CTDAC_VAL_NXT);
static void tcpwm_start(void);


/*******************************************************************************
 * Function Definitions
 *******************************************************************************/
/**
 * @brief Initialize CTDAC for speaker
 * 
 * @param void
 * @return void
 */
static void vdac_start(void)
{
    // Initialize and enable the analog reference block (AREF)
    Cy_SysAnalog_Init(&Cy_SysAnalog_Fast_Local);
    Cy_SysAnalog_Enable();

    // Configure CTDAC for VDDA reference and buffered output
    Cy_CTDAC_FastInit(CTDAC0, &Cy_CTDAC_Fast_VddaRef_BufferedOut);

    // Enable the CTDAC hardware block
    Cy_CTDAC_Enable(CTDAC0);

    // Set the clock divider to control the frequency of the DMA/CTDAC updates
    // (and the corresponding frequency of the sine wave). Start at nominal divider
    uint32_t currDiv = Cy_SysClk_PeriphGetAssignedDivider(PCLK_PASS_CLOCK_CTDAC);
    uint32_t newDiv = _VAL2FLD(PERI_DIV_CMD_TYPE_SEL, peri_0_div_24_5_0_HW)
                        | _VAL2FLD(PERI_DIV_CMD_DIV_SEL, peri_0_div_24_5_0_NUM);
    if (currDiv != newDiv)
    {
        (void)Cy_SysClk_PeriphDisableDivider(peri_0_div_24_5_0_HW, peri_0_div_24_5_0_NUM);
        (void)Cy_SysClk_PeriphSetFracDivider(peri_0_div_24_5_0_HW, peri_0_div_24_5_0_NUM, 199UL, 0UL);
        (void)Cy_SysClk_PeriphAssignDivider(PCLK_PASS_CLOCK_CTDAC, peri_0_div_24_5_0_HW, peri_0_div_24_5_0_NUM);
        (void)Cy_SysClk_PeriphEnableDivider(peri_0_div_24_5_0_HW, peri_0_div_24_5_0_NUM);
    }
}


/**
 * @brief  Initialize DMA for injecting audio samples into the CTDAC
 * 
 * @param sineWaveLUT
 * sine wave lookup table array
 * @param CTDAC_VAL_NXT
 * next memory address
 * @return void
 */
static void dma_start(uint32_t sineWaveLUT[], uint32_t *CTDAC_VAL_NXT)
{
    cy_en_dma_status_t dma_init_status;

    // Initialize descriptors and channel
    dma_init_status = Cy_DMA_Descriptor_Init(
        &cpuss_0_dw1_0_chan_1_Descriptor_0,
        &cpuss_0_dw1_0_chan_1_Descriptor_0_config
    );
    if (CY_DMA_SUCCESS != dma_init_status )
    {
        CY_ASSERT(0);
    }
    dma_init_status = Cy_DMA_Descriptor_Init(
        &cpuss_0_dw1_0_chan_1_Descriptor_1,
        &cpuss_0_dw1_0_chan_1_Descriptor_1_config
    );
    if (CY_DMA_SUCCESS != dma_init_status )
    {
        CY_ASSERT(0);
    }
    dma_init_status = Cy_DMA_Descriptor_Init(
        &cpuss_0_dw1_0_chan_1_Descriptor_2,
        &cpuss_0_dw1_0_chan_1_Descriptor_2_config
    );
    if (CY_DMA_SUCCESS != dma_init_status )
    {
        CY_ASSERT(0);
    }
    dma_init_status = Cy_DMA_Descriptor_Init(
        &cpuss_0_dw1_0_chan_1_Descriptor_3,
        &cpuss_0_dw1_0_chan_1_Descriptor_3_config
    );
    if (CY_DMA_SUCCESS != dma_init_status )
    {
        CY_ASSERT(0);
    }
    dma_init_status = Cy_DMA_Descriptor_Init(
        &cpuss_0_dw1_0_chan_1_Descriptor_4,
        &cpuss_0_dw1_0_chan_1_Descriptor_4_config
    );
    if (CY_DMA_SUCCESS != dma_init_status )
    {
        CY_ASSERT(0);
    }
    dma_init_status = Cy_DMA_Descriptor_Init(
        &cpuss_0_dw1_0_chan_1_Descriptor_5,
        &cpuss_0_dw1_0_chan_1_Descriptor_5_config
    );
    if (CY_DMA_SUCCESS != dma_init_status )
    {
        CY_ASSERT(0);
    }
    dma_init_status = Cy_DMA_Descriptor_Init(
        &cpuss_0_dw1_0_chan_1_Descriptor_6,
        &cpuss_0_dw1_0_chan_1_Descriptor_6_config
    );
    if (CY_DMA_SUCCESS != dma_init_status )
    {
        CY_ASSERT(0);
    }
    dma_init_status = Cy_DMA_Descriptor_Init(
        &cpuss_0_dw1_0_chan_1_Descriptor_7,
        &cpuss_0_dw1_0_chan_1_Descriptor_7_config
    );
    if (CY_DMA_SUCCESS != dma_init_status )
    {
        CY_ASSERT(0);
    }
    dma_init_status = Cy_DMA_Descriptor_Init(
        &cpuss_0_dw1_0_chan_1_Descriptor_8,
        &cpuss_0_dw1_0_chan_1_Descriptor_8_config
    );
    if (CY_DMA_SUCCESS != dma_init_status )
    {
        CY_ASSERT(0);
    }
    dma_init_status = Cy_DMA_Channel_Init(
        cpuss_0_dw1_0_chan_1_HW,
        cpuss_0_dw1_0_chan_1_CHANNEL,
        &cpuss_0_dw1_0_chan_1_channelConfig
    );
    if (CY_DMA_SUCCESS != dma_init_status )
    {
        CY_ASSERT(0);
    }

    // Set source addresses in the sample LUT for descriptors 0-8 to step through the samples sequentially
    Cy_DMA_Descriptor_SetSrcAddress(&cpuss_0_dw1_0_chan_1_Descriptor_0,
                                    (uint32_t*) soundByteSamplesLUT);
    Cy_DMA_Descriptor_SetSrcAddress(&cpuss_0_dw1_0_chan_1_Descriptor_1,
                                    (uint32_t*) (soundByteSamplesLUT + 256));
    Cy_DMA_Descriptor_SetSrcAddress(&cpuss_0_dw1_0_chan_1_Descriptor_2,
                                    (uint32_t*) (soundByteSamplesLUT + 512));
    Cy_DMA_Descriptor_SetSrcAddress(&cpuss_0_dw1_0_chan_1_Descriptor_3,
                                    (uint32_t*) (soundByteSamplesLUT + 768));
    Cy_DMA_Descriptor_SetSrcAddress(&cpuss_0_dw1_0_chan_1_Descriptor_4,
                                    (uint32_t*) (soundByteSamplesLUT + 1024));
    Cy_DMA_Descriptor_SetSrcAddress(&cpuss_0_dw1_0_chan_1_Descriptor_5,
                                    (uint32_t*) (soundByteSamplesLUT + 1280));
    Cy_DMA_Descriptor_SetSrcAddress(&cpuss_0_dw1_0_chan_1_Descriptor_6,
                                    (uint32_t*) (soundByteSamplesLUT + 1536));
    Cy_DMA_Descriptor_SetSrcAddress(&cpuss_0_dw1_0_chan_1_Descriptor_7,
                                    (uint32_t*) (soundByteSamplesLUT + 1792));
    Cy_DMA_Descriptor_SetSrcAddress(&cpuss_0_dw1_0_chan_1_Descriptor_8,
                                    (uint32_t*) (soundByteSamplesLUT + 2048));
    // Set destination addresses as the CTDAC buffer register
    Cy_DMA_Descriptor_SetDstAddress(&cpuss_0_dw1_0_chan_1_Descriptor_0,
                                    (uint32_t*) &(pass_0_ctdac_0_HW->CTDAC_VAL_NXT));
    Cy_DMA_Descriptor_SetDstAddress(&cpuss_0_dw1_0_chan_1_Descriptor_1,
                                    (uint32_t*) &(pass_0_ctdac_0_HW->CTDAC_VAL_NXT));
    Cy_DMA_Descriptor_SetDstAddress(&cpuss_0_dw1_0_chan_1_Descriptor_2,
                                    (uint32_t*) &(pass_0_ctdac_0_HW->CTDAC_VAL_NXT));
    Cy_DMA_Descriptor_SetDstAddress(&cpuss_0_dw1_0_chan_1_Descriptor_3,
                                    (uint32_t*) &(pass_0_ctdac_0_HW->CTDAC_VAL_NXT));
    Cy_DMA_Descriptor_SetDstAddress(&cpuss_0_dw1_0_chan_1_Descriptor_4,
                                    (uint32_t*) &(pass_0_ctdac_0_HW->CTDAC_VAL_NXT));
    Cy_DMA_Descriptor_SetDstAddress(&cpuss_0_dw1_0_chan_1_Descriptor_5,
                                    (uint32_t*) &(pass_0_ctdac_0_HW->CTDAC_VAL_NXT));
    Cy_DMA_Descriptor_SetDstAddress(&cpuss_0_dw1_0_chan_1_Descriptor_6,
                                    (uint32_t*) &(pass_0_ctdac_0_HW->CTDAC_VAL_NXT));
    Cy_DMA_Descriptor_SetDstAddress(&cpuss_0_dw1_0_chan_1_Descriptor_7,
                                    (uint32_t*) &(pass_0_ctdac_0_HW->CTDAC_VAL_NXT));
    Cy_DMA_Descriptor_SetDstAddress(&cpuss_0_dw1_0_chan_1_Descriptor_8,
                                    (uint32_t*) &(pass_0_ctdac_0_HW->CTDAC_VAL_NXT));

    // Initialize descriptor and channel
    dma_init_status = Cy_DMA_Descriptor_Init(
        &cpuss_0_dw1_0_chan_0_Descriptor_0,
        &cpuss_0_dw1_0_chan_0_Descriptor_0_config
    );
    if (CY_DMA_SUCCESS != dma_init_status )
    {
        CY_ASSERT(0);
    }
    dma_init_status = Cy_DMA_Channel_Init(
        cpuss_0_dw1_0_chan_0_HW,
        cpuss_0_dw1_0_chan_0_CHANNEL,
        &cpuss_0_dw1_0_chan_0_channelConfig
    );
    if (CY_DMA_SUCCESS != dma_init_status )
    {
        CY_ASSERT(0);
    }

    // Set source address as the LUT for descriptor 0
    Cy_DMA_Descriptor_SetSrcAddress(&cpuss_0_dw1_0_chan_0_Descriptor_0,
                                    (uint32_t*) sineWaveLUT);
    // Set destination address as the CTDAC buffer register
    Cy_DMA_Descriptor_SetDstAddress(&cpuss_0_dw1_0_chan_0_Descriptor_0,
                                    (uint32_t*) &(pass_0_ctdac_0_HW->CTDAC_VAL_NXT));

    // Enable the descriptor
    Cy_DMA_Channel_Enable(cpuss_0_dw1_0_chan_0_HW, cpuss_0_dw1_0_chan_0_CHANNEL);
    Cy_DMA_Enable(cpuss_0_dw1_0_chan_0_HW);
}


/**
 * @brief Initialize TCPWM for IR LED
 * 
 * @param void
 * @return void
 */
static void tcpwm_start(void)
{
    // NOTE: The TCPWM clock is set using the Device Configurator tool
    Cy_TCPWM_PWM_Init(tcpwm_0_cnt_5_HW, tcpwm_0_cnt_5_NUM, &tcpwm_0_cnt_5_config);
    Cy_TCPWM_PWM_Enable(tcpwm_0_cnt_5_HW, tcpwm_0_cnt_5_NUM);
}


int main(void)
{
    cy_rslt_t rslt;

    /* Initialize the device and board peripherals */
    rslt = cybsp_init();
    CY_ASSERT(rslt == CY_RSLT_SUCCESS);

    __enable_irq();
    
    // Initialize and start the CTDAC
    vdac_start();
    // Initialize and start the DMA
    dma_start(transformedSineWaveLUT, (uint32_t*) &(pass_0_ctdac_0_HW->CTDAC_VAL_NXT));
    // Initialize the PWM
    tcpwm_start();

    task_console_init();

    task_ir_led_init();
    task_audio_init();

    /* Start the FreeRTOS scheduler */
    vTaskStartScheduler();
    
    for (;;)
    {
    }
}

/* [] END OF FILE */