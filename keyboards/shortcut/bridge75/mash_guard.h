// Copyright 2025 emolitor (github.com/emolitor)
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <stdbool.h>
#include <stdint.h>

#define MASH_GUARD_MAX_KEYS 2
#define MASH_UNDO_MIN_KEYS 5

static inline bool mash_guarded_keycode(uint16_t kc) {
    return IS_QK_KB(kc) || IS_QK_USER(kc)
           || IS_UNDERGLOW_KEYCODE(kc) || IS_RGB_MATRIX_KEYCODE(kc)
           || IS_QK_MAGIC(kc)
           || IS_QK_DEF_LAYER(kc) || IS_QK_PERSISTENT_DEF_LAYER(kc)
           || kc == QK_BOOT || kc == QK_REBOOT || kc == QK_CLEAR_EEPROM;
}

static inline bool mash_guard_blocks(uint16_t kc, bool pressed, uint8_t keys_down) {
    return pressed && keys_down > MASH_GUARD_MAX_KEYS && mash_guarded_keycode(kc);
}

static inline bool mash_latch_next(bool latched, uint8_t keys_down) {
    if (keys_down > MASH_GUARD_MAX_KEYS) return true;
    if (keys_down <= 1) return false;
    return latched;
}
