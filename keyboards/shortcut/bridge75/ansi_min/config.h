// Copyright 2025 emolitor (github.com/emolitor)
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#define LED_ESC_INDEX 0
#define LED_F1_INDEX  1
#define LED_F12_INDEX 12
#define LED_DEL_INDEX 13

#define CAPSLOCK_INDEX LED_ESC_INDEX
#define ESCAPE_INDEX LED_ESC_INDEX
#define DEVS_BT1_INDEX 6
#define DEVS_BT2_INDEX 7
#define DEVS_BT3_INDEX 8
#define DEVS_2G4_INDEX 5

#define DYNAMIC_KEYMAP_LAYER_COUNT 6

#define BRIDGE75_RGB_HARD_TIMEOUT_MIN 30

#define EECONFIG_USER_DATA_SIZE 32

#undef MD_BT_NAME
#define MD_BT_NAME "Bridge75 Jscio BT$"

#define RGB_MATRIX_VAL_STEP 8

#undef RGB_MATRIX_LED_FLUSH_LIMIT
#define RGB_MATRIX_LED_FLUSH_LIMIT 16
