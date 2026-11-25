// Copyright 2025 emolitor (github.com/emolitor)
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "quantum.h"

// Keycodes are defined in info.json and auto-generated
// Available keycodes: BT_HOST1, BT_HOST2, BT_HOST3, BT_2_4G, BT_USB,
//                     BT_SW, BT_VOL, USBSLP, RGB_TEST
// Additional keycodes to add to info.json: LowD, ComD, HighD

#ifndef BRIDGE75_RGB_HARD_TIMEOUT_MIN
#    define BRIDGE75_RGB_HARD_TIMEOUT_MIN 0
#endif

extern bool     bridge75_cold_boot;
extern uint16_t bridge75_rgb_hard_timeout_min;
bool            bridge75_keep_awake(void);
bool            bridge75_mash_active(void);

#ifdef BRIDGE75_WAKE_REPLAY
extern bool bridge75_wake_replay;
#endif
