// Copyright 2025 emolitor (github.com/emolitor)
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "report.h"

#define WAKE_REPLAY_MAX 16
#define WAKE_REPLAY_WINDOW_MS 4000

void wake_replay_arm(uint32_t now);
void wake_replay_disarm(void);
bool wake_replay_armed(void);

bool wake_replay_capture(const report_keyboard_t *report, uint32_t now);

uint8_t wake_replay_poll(bool connected, uint32_t now, void (*send)(report_keyboard_t *report));
