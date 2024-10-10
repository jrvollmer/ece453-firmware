/**
 * @file main.h
 * @author James Vollmer (jrvollmer@wisc.edu) - Team 01
 * @brief 
 * @version 0.2
 * @date 2024-10-16
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#ifndef __MAIN_H__
#define __MAIN_H__

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
#include "source/app_hw/i2c.h"
#include "source/app_hw/spi.h"
#include "source/app_hw/task_console.h"
#include "source/app_hw/task_audio.h"
#include "source/app_hw/task_ir_led.h"
#include "source/app_hw/task_io_expander.h"

#endif