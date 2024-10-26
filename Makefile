################################################################################
# \file Makefile
# \version 1.0
#
# \brief
# Entry point for project make targets
#
################################################################################
# \copyright
# Copyright 2018-2024, Cypress Semiconductor Corporation (an Infineon company)
# SPDX-License-Identifier: Apache-2.0
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
################################################################################



################################################################################
# Basic Configuration
################################################################################

# Type of ModusToolbox Makefile Options include:
#
# COMBINED    -- Top Level Makefile usually for single standalone application
# APPLICATION -- Top Level Makefile usually for multi project application
# PROJECT     -- Project Makefile under Application
#
MTB_TYPE=COMBINED

# Target board/hardware (BSP).
# To change the target, it is recommended to use the Library manager
# ('make library-manager' from command line), which will also update Eclipse IDE launch
# configurations.
TARGET=APP_CY8CPROTO-063-BLE

# Name of application (used to derive name of final linked file).
APPNAME=bucky-kart

# Name of toolchain to use. Options include:
#
# GCC_ARM -- GCC provided with ModusToolbox software
# ARM     -- ARM Compiler (must be installed separately)
# IAR     -- IAR Compiler (must be installed separately)
#
# See also: CY_COMPILER_PATH below
TOOLCHAIN=GCC_ARM

# Default build configuration. Options include:
#
# Debug -- build with minimal optimizations, focus on debugging.
# Release -- build with full optimizations
# Custom -- build with custom configuration, set the optimization flag in CFLAGS
#
# If CONFIG is manually edited, ensure to update or regenerate launch configurations
# for your IDE.
CONFIG=Debug

# If set to "true" or "1", display full command-lines when building.
VERBOSE=

# Optionally enable app and Bluetooth protocol traces and route to BTSpy
# add airoc-hci-transport from library manager before enabling
ENABLE_SPY_TRACES = 0
# Specify the flash region to be used as NVRAM for bond data storage
USE_INTERNAL_FLASH = 0

ifeq ($(TARGET), $(filter $(TARGET), APP_CY8CKIT-062-BLE APP_CY8CPROTO-063-BLE APP_CYBLE-416045-EVAL))
PSOC6_BLE = 1
DEFINES+= PSOC6_BLE
ENABLE_SPY_TRACES = 0
USE_INTERNAL_FLASH = 1
endif

ifeq ($(TARGET), $(filter $(TARGET), APP_CY8CKIT-062-WIFI-BT APP_CYW9P62S1-43438EVB-01 APP_CYW9P62S1-43012EVB-01 APP_CY8CKIT-062S2-43012 APP_CY8CPROTO-062S3-4343W APP_CY8CEVAL-062S2-LAI-4373M2 APP_CY8CPROTO-062-4343W APP_CY8CEVAL-062S2-LAI-43439M2 APP_CY8CPROTO-062S2-43439 APP_CY8CEVAL-062S2-MUR-4373M2 APP_CY8CEVAL-062S2-MUR-4373EM2 APP_CY8CEVAL-062S2-CYW43022CUB))
USE_INTERNAL_FLASH = 1
endif

ifeq ($(ENABLE_SPY_TRACES),1)
DEFINES+=ENABLE_BT_SPY_LOG DEBUG_UART_BAUDRATE=3000000
else
DEFINES+=ENABLE_AIROC_HCI_TRANSPORT_PRINTF=0
endif

ifeq ($(USE_INTERNAL_FLASH),1)
DEFINES+=USE_INTERNAL_FLASH
endif


################################################################################
# Advanced Configuration
################################################################################

# Enable optional code that is ordinarily disabled by default.
#
# Available components depend on the specific targeted hardware and firmware
# in use. In general, if you have
#
#    COMPONENTS=foo bar
#
# ... then code in directories named COMPONENT_foo and COMPONENT_bar will be
# added to the build
#
COMPONENTS=FREERTOS WICED_BLE
# Like COMPONENTS, but disable optional code that was enabled by default.
DISABLE_COMPONENTS=

# By default the build system automatically looks in the Makefile's directory
# tree for source code and builds it. The SOURCES variable can be used to
# manually add source code to the build process from a location not searched
# by default, or otherwise not found by the build system.
SOURCES=

# Like SOURCES, but for include directories. Value should be paths to
# directories (without a leading -I).
INCLUDES=./configs

# Add additional defines to the build process (without a leading -D).
DEFINES+=CY_RETARGET_IO_CONVERT_LF_TO_CRLF CY_RTOS_AWARE

# Select softfp or hardfp floating point. Default is softfp.
VFP_SELECT=

# Additional / custom C compiler flags.
#
# NOTE: Includes and defines should use the INCLUDES and DEFINES variable
# above.
CFLAGS=

# Additional / custom C++ compiler flags.
#
# NOTE: Includes and defines should use the INCLUDES and DEFINES variable
# above.
CXXFLAGS=

# Additional / custom assembler flags.
#
# NOTE: Includes and defines should use the INCLUDES and DEFINES variable
# above.
ASFLAGS=

# Additional / custom linker flags.
LDFLAGS=

# Additional / custom libraries to link in to the application.
LDLIBS=

# Path to the linker script to use (if empty, use the default linker script).
LINKER_SCRIPT=

# Custom pre-build commands to run.
PREBUILD=

# Custom post-build commands to run.
POSTBUILD=


################################################################################
# Paths
################################################################################

# Relative path to the project directory (default is the Makefile's directory).
#
# This controls where automatic source code discovery looks for code.
CY_APP_PATH=

# Relative path to the shared repo location.
#
# All .mtb files have the format, <URI>#<COMMIT>#<LOCATION>. If the <LOCATION> field
# begins with $$ASSET_REPO$$, then the repo is deposited in the path specified by
# the CY_GETLIBS_SHARED_PATH variable. The default location is one directory level
# above the current app directory.
# This is used with CY_GETLIBS_SHARED_NAME variable, which specifies the directory name.
CY_GETLIBS_SHARED_PATH=../

# Directory name of the shared repo location.
#
CY_GETLIBS_SHARED_NAME=mtb_shared

# Absolute path to the compiler's "bin" directory. The variable name depends on the
# toolchain used for the build. Refer to the ModusToolbox user guide to get the correct
# variable name for the toolchain used in your build.
#
# The default depends on the selected TOOLCHAIN (GCC_ARM uses the ModusToolbox
# software provided compiler by default).
CY_COMPILER_GCC_ARM_DIR=


# Locate ModusToolbox helper tools folders in default installation
# locations for Windows, Linux, and macOS.
CY_WIN_HOME=$(subst \,/,$(USERPROFILE))
CY_TOOLS_PATHS ?= $(wildcard \
    $(CY_WIN_HOME)/ModusToolbox/tools_* \
    $(HOME)/ModusToolbox/tools_* \
    /Applications/ModusToolbox/tools_*)

# If you install ModusToolbox software in a custom location, add the path to its
# "tools_X.Y" folder (where X and Y are the version number of the tools
# folder). Make sure you use forward slashes.
CY_TOOLS_PATHS+=

# Default to the newest installed tools folder, or the users override (if it's
# found).
CY_TOOLS_DIR=$(lastword $(sort $(wildcard $(CY_TOOLS_PATHS))))

ifeq ($(CY_TOOLS_DIR),)
$(error Unable to find any of the available CY_TOOLS_PATHS -- $(CY_TOOLS_PATHS). On Windows, use forward slashes.)
endif

# Entrypoint for underlying make targets
$(info Tools Directory: $(CY_TOOLS_DIR))

include $(CY_TOOLS_DIR)/make/start.mk

# Luckily, the 'prebuild' target is already defined, so we can use it to modify generated source
# code before building (yay for making BLE server names generated constants)
prebuild:
ifdef CAR
	@echo -e '\n\033[0;33mInjecting malware into generated source code\033[0m\n'
	@python3 rename_car.py $(CAR)
endif

# What we have below is some fun Makefile magic to show a splash screen after building or
# programming. Unfortunately, the default 'build' and 'program' targets are in an untracked file
ifeq (${IS_MAKEFILE_RUNNING_TARGETS},)

# Just a flag to set once we've made the initial call to 'build' or 'program'. The target in the
# 'else' block below runs at the end of the make invocation
define DEFAULTTARGET := ;
endef

%:
	@:
	$(if ${IS_MAKEFILE_RUNNING_TARGETS},,${DEFAULTTARGET})
	$(eval export IS_MAKEFILE_RUNNING_TARGETS=1)

else

program build &:
	@cat splash.txt

endif
