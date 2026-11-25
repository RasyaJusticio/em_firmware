// Copyright 2025 emolitor (github.com/emolitor)
// SPDX-License-Identifier: GPL-2.0-or-later

#include "wake_replay.h"
#include <string.h>

static report_keyboard_t queue[WAKE_REPLAY_MAX];
static uint8_t           count;
static bool              armed;
static uint32_t          armed_at;

void wake_replay_arm(uint32_t now) {
    armed    = true;
    armed_at = now;
    count    = 0;
}

void wake_replay_disarm(void) {
    armed = false;
    count = 0;
}

bool wake_replay_armed(void) {
    return armed;
}

bool wake_replay_capture(const report_keyboard_t *report, uint32_t now) {
    if (!armed || report == NULL || (uint32_t)(now - armed_at) >= WAKE_REPLAY_WINDOW_MS) {
        return false;
    }
    if (count < WAKE_REPLAY_MAX) {
        queue[count++] = *report;
    } else {
        queue[WAKE_REPLAY_MAX - 1] = *report;
    }
    return true;
}

uint8_t wake_replay_poll(bool connected, uint32_t now, void (*send)(report_keyboard_t *report)) {
    if (!armed) {
        return 0;
    }
    if (!connected) {
        if ((uint32_t)(now - armed_at) >= WAKE_REPLAY_WINDOW_MS) {
            wake_replay_disarm();
        }
        return 0;
    }
    uint8_t sent = count;
    for (uint8_t i = 0; i < count; i++) {
        send(&queue[i]);
    }
    wake_replay_disarm();
    return sent;
}
