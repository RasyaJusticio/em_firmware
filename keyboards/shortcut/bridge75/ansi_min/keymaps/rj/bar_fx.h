// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "color.h"

#define BFX_LEDS 14

enum bfx_ids {
    BFX_AURORA,
    BFX_OCEAN,
    BFX_TWINKLE,
    BFX_EMBER,
    BFX_LAVA,
    BFX_BREATH,
    BFX_COLORWAVES,
    BFX_RIPPLES,
    BFX_GLOW,
    BFX_COUNT
};

void bfx_render(uint8_t id, uint32_t now_ms, uint8_t hue, uint8_t sat, uint8_t speed, rgb_t out[BFX_LEDS]);

void bfx_key_hit(uint8_t x);

bool bfx_is_reactive(uint8_t id);
