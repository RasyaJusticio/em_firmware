// SPDX-License-Identifier: GPL-2.0-or-later
RGB_MATRIX_EFFECT(BAR_AURORA)
RGB_MATRIX_EFFECT(BAR_OCEAN)
RGB_MATRIX_EFFECT(BAR_TWINKLE)
RGB_MATRIX_EFFECT(BAR_EMBER)
RGB_MATRIX_EFFECT(BAR_LAVA)
RGB_MATRIX_EFFECT(BAR_BREATH)
RGB_MATRIX_EFFECT(BAR_COLORWAVES)
RGB_MATRIX_EFFECT(BAR_RIPPLES)
RGB_MATRIX_EFFECT(BAR_GLOW)

#ifdef RGB_MATRIX_CUSTOM_EFFECT_IMPLS
#    include "bar_fx.h"

static bool bar_fx_run(effect_params_t *params, uint8_t id) {
    static rgb_t frame[BFX_LEDS];
    RGB_MATRIX_USE_LIMITS(led_min, led_max);
    if (led_min == 0) {
        bfx_render(id, g_rgb_timer, rgb_matrix_config.hsv.h, rgb_matrix_config.hsv.s, rgb_matrix_config.speed, frame);
    }
    uint8_t v = rgb_matrix_config.hsv.v;
    for (uint8_t i = led_min; i < led_max; i++) {
        RGB_MATRIX_TEST_LED_FLAGS();
        if (i >= BFX_LEDS) continue;
        rgb_matrix_set_color(i, scale8(frame[i].r, v), scale8(frame[i].g, v), scale8(frame[i].b, v));
    }
    return rgb_matrix_check_finished_leds(led_max);
}

static bool BAR_AURORA(effect_params_t *params) { return bar_fx_run(params, BFX_AURORA); }
static bool BAR_OCEAN(effect_params_t *params) { return bar_fx_run(params, BFX_OCEAN); }
static bool BAR_TWINKLE(effect_params_t *params) { return bar_fx_run(params, BFX_TWINKLE); }
static bool BAR_EMBER(effect_params_t *params) { return bar_fx_run(params, BFX_EMBER); }
static bool BAR_LAVA(effect_params_t *params) { return bar_fx_run(params, BFX_LAVA); }
static bool BAR_BREATH(effect_params_t *params) { return bar_fx_run(params, BFX_BREATH); }
static bool BAR_COLORWAVES(effect_params_t *params) { return bar_fx_run(params, BFX_COLORWAVES); }
static bool BAR_RIPPLES(effect_params_t *params) { return bar_fx_run(params, BFX_RIPPLES); }
static bool BAR_GLOW(effect_params_t *params) { return bar_fx_run(params, BFX_GLOW); }
#endif
