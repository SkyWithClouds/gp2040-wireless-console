/* SPDX-License-Identifier: MIT */
#pragma once
#include "../Blank/BoardConfig.h"
#undef BOARD_CONFIG_LABEL
#define BOARD_CONFIG_LABEL "WirelessRxBox"
#define USB_PERIPHERAL_ENABLED 1
#define USB_PERIPHERAL_PIN_DPLUS 0
#define USB_PERIPHERAL_PIN_ORDER 0
#define USB_PERIPHERAL_PIN_5V -1
#define DEFAULT_PS4AUTHENTICATION_TYPE INPUT_MODE_AUTH_TYPE_USB
#define DEFAULT_PS5AUTHENTICATION_TYPE INPUT_MODE_AUTH_TYPE_USB
#define DEFAULT_SOCD_MODE SOCD_MODE_BYPASS
#define DEFAULT_INPUT_MODE INPUT_MODE_XINPUT
// Unassigned GPIOs inherit Blank's NONE mapping. Keep bus/maintenance pins out of profiles.
#define GPIO_PIN_00 GpioAction::RESERVED
#define GPIO_PIN_01 GpioAction::RESERVED
#define GPIO_PIN_16 GpioAction::RESERVED
#define GPIO_PIN_17 GpioAction::RESERVED
#define GPIO_PIN_18 GpioAction::RESERVED
#define GPIO_PIN_19 GpioAction::RESERVED
#define GPIO_PIN_20 GpioAction::RESERVED
#define GPIO_PIN_22 GpioAction::RESERVED
