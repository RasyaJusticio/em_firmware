// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "color.h"

#define ANIM_LEDS 14
#define ANIM_LEVEL 200

enum anim_ids {
    ANIM_OFF,
    ANIM_COMETS,
    ANIM_RAINBOW_WIPE,
    ANIM_LARSON,
    ANIM_METEOR,
    ANIM_CURTAIN,
    ANIM_IGNITION,
    ANIM_FIRE,
    ANIM_POWER_UP,
    ANIM_WELCOME,
    ANIM_RIPPLE,
    ANIM_SUNRISE,
    ANIM_BOUNCE,
    ANIM_HEARTBEAT,
    ANIM_SHUFFLE,
    ANIM_COUNT
};

#define ANIM_FIRST_REAL ANIM_COMETS
#define ANIM_LAST_REAL ANIM_HEARTBEAT

enum anim_color_modes {
    ANIM_COLOR_AUTO,
    ANIM_COLOR_PROFILE,
    ANIM_COLOR_CUSTOM,
    ANIM_COLOR_RAINBOW,
    ANIM_COLOR_COUNT
};

void anim_start(uint8_t id, uint8_t color_mode, hsv_t profile, hsv_t custom, uint32_t seed);

bool anim_render(uint32_t t, rgb_t out[ANIM_LEDS]);

uint16_t anim_duration(uint8_t id);
