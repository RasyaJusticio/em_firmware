// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H
#include "bridge75.h"
#include "color.h"
#include "via.h"
#include "startup_anim.h"
#include "bar_fx.h"
#include "bar_keypos.h"
#include "dynamic_keymap.h"
#include "ws2812.h"
#include <string.h>

extern ws2812_led_t ws2812_leds[];

#ifdef WIRELESS_ENABLE
#    include "wireless.h"
#endif

// fn layers must stay above every base layer, else fn get hidden after profile switch
enum layers {
    PUB_B, PER_B, GAM_B,
    PUB_FN, PER_FN, GAM_FN,
};

#define PROFILE_COUNT 3
#define FN_LAYER_MASK ((1UL << PUB_FN) | (1UL << PER_FN) | (1UL << GAM_FN))

enum profiles { PROFILE_PUBLIC, PROFILE_PERSONAL, PROFILE_GAMING };

enum custom_keycodes {
    PRF_NXT = QK_KB_12,
    PRF_PUB,
    PRF_PER,
    PRF_GAM,
    FX_TOG,
    ANIM_PLAY,
};
_Static_assert(PRF_NXT == RGB_TEST + 1, "custom keycodes must follow the keyboard level ones");

enum socd_modes { SOCD_LAST, SOCD_NEUTRAL, SOCD_FIRST, SOCD_OFF, SOCD_MODE_COUNT };
enum rj_flags {
    PF_IDLE_DIM    = 1 << 0,
    PF_HUD         = 1 << 1,
    PF_SWEEP       = 1 << 2,
    PF_CONN_PULSE  = 1 << 3,
    PF_SLEEP_FADE  = 1 << 4,
    PF_WAKE_REPLAY = 1 << 5,
    PF_ALL         = 0x3F,
};
#define IDLE_DIM_S_MIN 5
#define IDLE_DIM_S_MAX 50
enum del_modes  { DEL_FN_ONLY, DEL_ALWAYS, DEL_MODE_COUNT };

// clang-format off
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [PUB_B] = LAYOUT_ansi(
        KC_ESC,  KC_F1,   KC_F2,   KC_F3,   KC_F4,   KC_F5,   KC_F6,   KC_F7,   KC_F8,   KC_F9,   KC_F10,  KC_F11,  KC_F12,                    KC_DEL,
        KC_GRV,  KC_1,    KC_2,    KC_3,    KC_4,    KC_5,    KC_6,    KC_7,    KC_8,    KC_9,    KC_0,    KC_MINS, KC_EQL,  KC_BSPC,          KC_HOME,
        KC_TAB,  KC_Q,    KC_W,    KC_E,    KC_R,    KC_T,    KC_Y,    KC_U,    KC_I,    KC_O,    KC_P,    KC_LBRC, KC_RBRC, KC_BSLS,          KC_PGUP,
        KC_CAPS, KC_A,    KC_S,    KC_D,    KC_F,    KC_G,    KC_H,    KC_J,    KC_K,    KC_L,    KC_SCLN, KC_QUOT,          KC_ENT,           KC_PGDN,
        KC_LSFT, KC_Z,    KC_X,    KC_C,    KC_V,    KC_B,    KC_N,    KC_M,    KC_COMM, KC_DOT,  KC_SLSH,                   KC_RSFT, KC_UP,   KC_END,
        KC_LCTL, KC_LGUI, KC_LALT,                            KC_SPC,                             KC_RALT, MO(PUB_FN),       KC_LEFT, KC_DOWN, KC_RGHT
    ),

    [PER_B] = LAYOUT_ansi(
        KC_ESC,  KC_F1,   KC_F2,   KC_F3,   KC_F4,   KC_F5,   KC_F6,   KC_F7,   KC_F8,   KC_F9,   KC_F10,  KC_F11,  KC_F12,                    KC_DEL,
        KC_GRV,  KC_1,    KC_2,    KC_3,    KC_4,    KC_5,    KC_6,    KC_7,    KC_8,    KC_9,    KC_0,    KC_MINS, KC_EQL,  KC_BSPC,          KC_HOME,
        KC_TAB,  KC_Q,    KC_W,    KC_E,    KC_R,    KC_T,    KC_Y,    KC_U,    KC_I,    KC_O,    KC_P,    KC_LBRC, KC_RBRC, KC_BSLS,          KC_PGUP,
        KC_LCTL, KC_A,    KC_S,    KC_D,    KC_F,    KC_G,    KC_H,    KC_J,    KC_K,    KC_L,    KC_SCLN, KC_QUOT,          KC_ENT,           KC_PGDN,
        KC_LSFT, KC_Z,    KC_X,    KC_C,    KC_V,    KC_B,    KC_N,    KC_M,    KC_COMM, KC_DOT,  KC_SLSH,                   KC_RSFT, KC_UP,   KC_END,
        KC_CAPS, KC_LGUI, KC_LALT,                            KC_SPC,                             KC_RALT, MO(PER_FN),       KC_LEFT, KC_DOWN, KC_RGHT
    ),

    [GAM_B] = LAYOUT_ansi(
        KC_ESC,  KC_F1,   KC_F2,   KC_F3,   KC_F4,   KC_F5,   KC_F6,   KC_F7,   KC_F8,   KC_F9,   KC_F10,  KC_F11,  KC_F12,                    KC_DEL,
        KC_GRV,  KC_1,    KC_2,    KC_3,    KC_4,    KC_5,    KC_6,    KC_7,    KC_8,    KC_9,    KC_0,    KC_MINS, KC_EQL,  KC_BSPC,          KC_HOME,
        KC_TAB,  KC_Q,    KC_W,    KC_E,    KC_R,    KC_T,    KC_Y,    KC_U,    KC_I,    KC_O,    KC_P,    KC_LBRC, KC_RBRC, KC_BSLS,          KC_PGUP,
        KC_LCTL, KC_A,    KC_S,    KC_D,    KC_F,    KC_G,    KC_H,    KC_J,    KC_K,    KC_L,    KC_SCLN, KC_QUOT,          KC_ENT,           KC_PGDN,
        KC_LSFT, KC_Z,    KC_X,    KC_C,    KC_V,    KC_B,    KC_N,    KC_M,    KC_COMM, KC_DOT,  KC_SLSH,                   KC_RSFT, KC_UP,   KC_END,
        KC_CAPS, KC_LGUI, KC_LALT,                            KC_SPC,                             KC_RALT, MO(GAM_FN),       KC_LEFT, KC_DOWN, KC_RGHT
    ),

    [PUB_FN] = LAYOUT_ansi(
        _______, KC_BRID, KC_BRIU, KC_VOLD, KC_VOLU, KC_MUTE, KC_MPRV, KC_MPLY, KC_MNXT, RGB_VAD, RGB_VAI, FX_TOG,  KC_PSCR,                   KC_INS,
        BT_SW,   BT_HOST1, BT_HOST2, BT_HOST3, BT_2_4G, _______, _______, _______, _______, _______, _______, _______, _______, _______,          PRF_NXT,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, USBSLP,           _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,          _______,          _______,
        _______, RGB_RMOD, RGB_MOD, RGB_HUD, RGB_HUI, RGB_SPD, RGB_SPI, _______, _______, _______, _______,                   _______, _______, QK_REBOOT,
        _______, _______, _______,                            _______,                            _______, _______,          _______, _______, _______
    ),

    [PER_FN] = LAYOUT_ansi(
        _______, KC_BRID, KC_BRIU, KC_VOLD, KC_VOLU, KC_MUTE, KC_MPRV, KC_MPLY, KC_MNXT, RGB_VAD, RGB_VAI, FX_TOG,  KC_PSCR,                   KC_INS,
        BT_SW,   BT_HOST1, BT_HOST2, BT_HOST3, BT_2_4G, _______, _______, _______, _______, _______, _______, _______, _______, _______,          PRF_NXT,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, USBSLP,           _______,
        _______, _______, _______, _______, _______, _______, KC_LEFT, KC_DOWN, KC_UP,   KC_RGHT, _______, _______,          _______,          _______,
        _______, RGB_RMOD, RGB_MOD, RGB_HUD, RGB_HUI, RGB_SPD, RGB_SPI, _______, _______, _______, _______,                   _______, _______, QK_REBOOT,
        _______, _______, _______,                            _______,                            _______, _______,          _______, _______, _______
    ),

    [GAM_FN] = LAYOUT_ansi(
        _______, KC_BRID, KC_BRIU, KC_VOLD, KC_VOLU, KC_MUTE, KC_MPRV, KC_MPLY, KC_MNXT, RGB_VAD, RGB_VAI, FX_TOG,  KC_PSCR,                   KC_INS,
        BT_SW,   BT_HOST1, BT_HOST2, BT_HOST3, BT_2_4G, _______, _______, _______, _______, _______, _______, _______, _______, _______,          PRF_NXT,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, USBSLP,           _______,
        _______, _______, _______, _______, _______, _______, KC_LEFT, KC_DOWN, KC_UP,   KC_RGHT, _______, _______,          _______,          _______,
        _______, RGB_RMOD, RGB_MOD, RGB_HUD, RGB_HUI, RGB_SPD, RGB_SPI, _______, _______, _______, _______,                   _______, _______, QK_REBOOT,
        _______, _______, _______,                            _______,                            _______, _______,          _______, _______, _______
    ),
};
// clang-format on

typedef struct __attribute__((packed)) {
    uint8_t profile;
    uint8_t del_mode;
    uint8_t socd_mode;
    uint8_t startup_anim;
    uint8_t fx_on;
    uint8_t rgb_off_min;
    uint8_t hue[PROFILE_COUNT];
    uint8_t sat[PROFILE_COUNT];
    uint8_t anim_color;
    uint8_t anim_hue;
    uint8_t anim_sat;
    uint8_t shuffle_next;
    uint8_t flags;
    uint8_t idle_dim_pct;
    uint8_t idle_dim_s;
    uint8_t fn_dim_pct;
    uint8_t nav;
    uint8_t rev;
    uint8_t reserved[10];
} rj_config_t;

#define PERSONAL_CONFIG_V1_SIZE 16
#define CFG_REV 3
#define JSCIO_SPEED 191

_Static_assert(sizeof(rj_config_t) == EECONFIG_USER_DATA_SIZE, "rj_config_t must match EECONFIG_USER_DATA_SIZE");

static rj_config_t cfg;

static const rj_config_t cfg_defaults = {
    .profile      = PROFILE_PUBLIC,
    .del_mode     = DEL_FN_ONLY,
    .socd_mode    = SOCD_LAST,
    .startup_anim = ANIM_WELCOME,
    .fx_on        = 1,
    .rgb_off_min  = BRIDGE75_RGB_HARD_TIMEOUT_MIN,
    .hue          = {170, 200, 0},
    .sat          = {255, 255, 255},
    .anim_color   = ANIM_COLOR_CUSTOM,
    .anim_hue     = 128,
    .anim_sat     = 255,
    .shuffle_next = ANIM_FIRST_REAL,
    .flags        = PF_ALL,
    .idle_dim_pct = 30,
    .idle_dim_s   = 30,
    .fn_dim_pct   = 25,
    .rev          = CFG_REV,
};

static void cfg_save(void) {
    eeconfig_update_user_datablock(&cfg);
}

static void cfg_sanitize(void) {
    if (cfg.profile >= PROFILE_COUNT) cfg.profile = PROFILE_PUBLIC;
    if (cfg.del_mode >= DEL_MODE_COUNT) cfg.del_mode = DEL_FN_ONLY;
    if (cfg.socd_mode >= SOCD_MODE_COUNT) cfg.socd_mode = SOCD_LAST;
    if (cfg.startup_anim >= ANIM_COUNT) cfg.startup_anim = ANIM_WELCOME;
    if (cfg.anim_color >= ANIM_COLOR_COUNT) cfg.anim_color = ANIM_COLOR_AUTO;
    if (cfg.shuffle_next < ANIM_FIRST_REAL || cfg.shuffle_next > ANIM_LAST_REAL) cfg.shuffle_next = ANIM_FIRST_REAL;
    cfg.flags &= PF_ALL;
    if (cfg.idle_dim_pct < 5 || cfg.idle_dim_pct > 95) cfg.idle_dim_pct = 30;
    if (cfg.idle_dim_s < IDLE_DIM_S_MIN || cfg.idle_dim_s > IDLE_DIM_S_MAX) cfg.idle_dim_s = 30;
    if (cfg.fn_dim_pct > 100) cfg.fn_dim_pct = 25;
    cfg.nav &= 0x0F;
    if (((cfg.nav >> 2) & 3) >= 3) cfg.nav &= 0x03;
    cfg.fx_on        = !!cfg.fx_on;
}

static void cfg_apply(void);

void eeconfig_init_user(void) {
    cfg = cfg_defaults;
    cfg_save();
    cfg_apply();
}

typedef struct {
    uint8_t key[2];
    bool    held[2];
    bool    sent[2];
    int8_t  first;
    int8_t  last;
} socd_axis_t;

static socd_axis_t socd_axes[2] = {
    {.key = {KC_A, KC_D}, .first = -1, .last = -1},
    {.key = {KC_W, KC_S}, .first = -1, .last = -1},
};

static void socd_resolve(socd_axis_t *ax) {
    bool want[2] = {ax->held[0], ax->held[1]};

    if (ax->held[0] && ax->held[1]) {
        switch (cfg.socd_mode) {
            case SOCD_NEUTRAL:
                want[0] = want[1] = false;
                break;
            case SOCD_FIRST:
                want[1 - ax->first] = false;
                break;
            default:
                want[1 - ax->last] = false;
                break;
        }
    }

    // release go out first, host must never see both direction down
    for (uint8_t i = 0; i < 2; i++) {
        if (!want[i] && ax->sent[i]) {
            ax->sent[i] = false;
            unregister_code(ax->key[i]);
        }
    }
    for (uint8_t i = 0; i < 2; i++) {
        if (want[i] && !ax->sent[i]) {
            ax->sent[i] = true;
            register_code(ax->key[i]);
        }
    }
}

static void socd_release_all(void) {
    for (uint8_t a = 0; a < 2; a++) {
        socd_axis_t *ax = &socd_axes[a];
        for (uint8_t i = 0; i < 2; i++) {
            if (ax->sent[i]) {
                unregister_code(ax->key[i]);
            }
            ax->sent[i] = ax->held[i] = false;
        }
        ax->first = ax->last = -1;
    }
}

static bool socd_process(uint16_t keycode, bool pressed) {
    if (cfg.profile != PROFILE_GAMING || cfg.socd_mode == SOCD_OFF) {
        return false;
    }
    for (uint8_t a = 0; a < 2; a++) {
        socd_axis_t *ax = &socd_axes[a];
        for (uint8_t i = 0; i < 2; i++) {
            if (keycode == ax->key[i]) {
                if (!pressed && !ax->held[i]) {
                    // pressed before socd take over, qmk registered it so qmk release it
                    return false;
                }
                ax->held[i] = pressed;
                if (pressed) {
                    ax->last = i;
                    if (!ax->held[1 - i]) {
                        ax->first = i;
                    }
                } else if (ax->held[1 - i]) {
                    ax->first = ax->last = 1 - i;
                }
                socd_resolve(ax);
                return true;
            }
        }
    }
    return false;
}

static uint32_t profile_flash_timer  = 0;
static bool     profile_flash_active = false;
static bool     del_flicker_active   = false;
static uint32_t del_flicker_timer    = 0;

static void sweep_start(void);
static void rgb_key_event(uint16_t kc, bool pressed);
static void rgb_repeat_task(void);

#define NAV_LINGER_MS 700

enum nav_bits { NAV_OFF = 1 << 0, NAV_NO_ECHO = 1 << 1 };
enum nav_fx_modes { NAV_FX_DIM, NAV_FX_NORMAL, NAV_FX_OFF, NAV_FX_COUNT };
#define NAV_FX(c) (((c) >> 2) & 3)

typedef struct {
    uint16_t keycode;
    uint8_t  echo_led;
} nav_key_t;

static const nav_key_t nav_keys[] = {
    {KC_LEFT, LED_F1_INDEX + 4},
    {KC_DOWN, LED_F1_INDEX + 5},
    {KC_UP, LED_F1_INDEX + 6},
    {KC_RGHT, LED_F1_INDEX + 7},
};
#define NAV_KEY_COUNT (sizeof nav_keys / sizeof nav_keys[0])

static uint8_t  nav_held          = 0;
static bool     nav_recent        = false;
static uint32_t nav_release_timer = 0;

static void nav_key_event(uint16_t keycode, bool pressed) {
    for (uint8_t i = 0; i < NAV_KEY_COUNT; i++) {
        if (nav_keys[i].keycode != keycode) continue;
        if (pressed) {
            if (layer_state & FN_LAYER_MASK) nav_held |= 1 << i;
        } else if (nav_held & (1 << i)) {
            nav_held &= ~(1 << i);
            nav_recent        = true;
            nav_release_timer = timer_read32();
        }
        return;
    }
}

static bool nav_focus_active(void) {
    if ((cfg.nav & NAV_OFF) || !(layer_state & FN_LAYER_MASK)) {
        nav_recent = false;
        return false;
    }
    if (nav_held) return true;
    if (nav_recent && timer_elapsed32(nav_release_timer) < NAV_LINGER_MS) return true;
    nav_recent = false;
    return false;
}

static void profile_apply(uint8_t profile, bool announce) {
    socd_release_all();
    cfg.profile = profile;
    default_layer_set(1UL << (PUB_B + profile));
    if (announce) {
        profile_flash_timer  = timer_read32();
        profile_flash_active = true;
        del_flicker_active   = true;
        del_flicker_timer    = profile_flash_timer;
        sweep_start();
    }
}

static void cfg_apply(void) {
    bridge75_rgb_hard_timeout_min = cfg.rgb_off_min;
#ifdef BRIDGE75_WAKE_REPLAY
    bridge75_wake_replay = cfg.flags & PF_WAKE_REPLAY;
#endif
    profile_apply(cfg.profile, false);
}

static void profile_select(uint8_t profile) {
    if (profile != cfg.profile) {
        profile_apply(profile, true);
        cfg_save();
    } else {
        profile_flash_timer  = timer_read32();
        profile_flash_active = true;
        del_flicker_active   = true;
        del_flicker_timer    = profile_flash_timer;
    }
}

#define FADE_MS 900

static bool     anim_active  = false;
static uint32_t anim_timer   = 0;
static bool     fade_armed   = false;
static bool     fade_pending = false;
static bool     fade_active  = false;
static uint32_t fade_timer   = 0;
static uint8_t  fade_target  = 0;
static uint8_t  fade_last    = 0;

static void startup_play(uint8_t id) {
    if (id == ANIM_OFF || !rgb_matrix_is_enabled()) return;
    if (id == ANIM_SHUFFLE) {
        id               = cfg.shuffle_next;
        cfg.shuffle_next = id >= ANIM_LAST_REAL ? ANIM_FIRST_REAL : id + 1;
        cfg_save();
    }

    hsv_t profile = {cfg.hue[cfg.profile], cfg.sat[cfg.profile], 255};
    hsv_t custom  = {cfg.anim_hue, cfg.anim_sat, 255};
    anim_start(id, cfg.anim_color, profile, custom, (timer_read32() + 1) * 2654435761u ^ ((uint32_t)cfg.shuffle_next << 16) ^ id);

    if (!anim_active && !fade_active) {
        fade_target = rgb_matrix_get_val();
    }
    fade_active  = false;
    fade_pending = false;
    fade_armed   = cfg.fx_on;
    if (fade_armed) {
        rgb_matrix_sethsv_noeeprom(rgb_matrix_get_hue(), rgb_matrix_get_sat(), 0);
        fade_last = 0;
    }
    anim_active = true;
    anim_timer  = timer_read32();
}

static void startup_restore_brightness(void) {
    if ((anim_active && fade_armed) || fade_pending || fade_active) {
        rgb_matrix_sethsv_noeeprom(rgb_matrix_get_hue(), rgb_matrix_get_sat(), fade_target);
    }
    fade_armed = fade_pending = fade_active = false;
}

void housekeeping_task_user(void) {
    rgb_repeat_task();
    if (anim_active && fade_armed && rgb_matrix_get_val() != 0) {
        fade_armed = false;
    }
    if (fade_pending) {
        fade_pending = false;
        fade_active  = true;
        fade_timer   = timer_read32();
    }
    if (fade_active) {
        if (rgb_matrix_get_val() != fade_last) {
            fade_active = false;
            return;
        }
        uint32_t e = timer_elapsed32(fade_timer);
        uint32_t p = e >= FADE_MS ? 1024 : e * 1024 / FADE_MS;
        uint32_t k = p * p * (3 * 1024 - 2 * p) / (1024 * 1024);
        uint8_t  v = (uint8_t)((uint32_t)fade_target * k * k / (1024 * 1024));
        rgb_matrix_sethsv_noeeprom(rgb_matrix_get_hue(), rgb_matrix_get_sat(), v);
        fade_last = rgb_matrix_get_val();
        if (e >= FADE_MS) fade_active = false;
    }
}

_Static_assert(MATRIX_ROWS == 6 && MATRIX_COLS == 15, "key_bar_x must match the matrix");

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (record->event.pressed && record->event.key.row < MATRIX_ROWS && record->event.key.col < MATRIX_COLS) {
        uint8_t x = key_bar_x[record->event.key.row][record->event.key.col];
        if (x != 255) bfx_key_hit(x);
    }

    switch (keycode) {
        case PRF_NXT:
            if (record->event.pressed) profile_select((cfg.profile + 1) % PROFILE_COUNT);
            return false;
        case PRF_PUB:
            if (record->event.pressed) profile_select(PROFILE_PUBLIC);
            return false;
        case PRF_PER:
            if (record->event.pressed) profile_select(PROFILE_PERSONAL);
            return false;
        case PRF_GAM:
            if (record->event.pressed) profile_select(PROFILE_GAMING);
            return false;
        case FX_TOG:
            if (record->event.pressed) {
                cfg.fx_on = !cfg.fx_on;
                cfg_save();
            }
            return false;
        case ANIM_PLAY:
            if (record->event.pressed) startup_play(cfg.startup_anim == ANIM_OFF ? ANIM_WELCOME : cfg.startup_anim);
            return false;
    }

    if (IS_UNDERGLOW_KEYCODE(keycode) || IS_RGB_MATRIX_KEYCODE(keycode)) {
        if (record->event.pressed) {
            startup_restore_brightness();
        }
        rgb_key_event(keycode, record->event.pressed);
    }

    nav_key_event(keycode, record->event.pressed);
    return !socd_process(keycode, record->event.pressed);
}

#define WHITE_LEVEL 200
#define ROW_LEDS 14

#define MIX_IN_MS 120
#define MIX_OUT_MS 220
#define FAST_IN_MS 40
#define FAST_OUT_MS 160
#define FX_FADE_MS 300
#define FN_DIM_IN_MS 150
#define FN_DIM_OUT_MS 250
#define IDLE_DIM_MS 1500
#define IDLE_OFF_START 58000
#define IDLE_OFF_MS 2000
#define IDLE_WAKE_MS 200

typedef struct {
    uint8_t r, g, b;
} col_t;

static col_t    req_col[ROW_LEDS], mix_col[ROW_LEDS];
static uint8_t  req_a[ROW_LEDS];
static bool     req_snap[ROW_LEDS];
static bool     req_exact[ROW_LEDS];
static bool     req_fast[ROW_LEDS];
static bool     mix_fast[ROW_LEDS];
static uint16_t mix_a[ROW_LEDS];
static uint16_t eff_gain  = 65535;
static uint16_t idle_gain = 65535;
static uint32_t mix_last  = 0;
static uint8_t  led_lo, led_hi;

static ws2812_led_t eff_frame[ROW_LEDS];
static ws2812_led_t out_last[ROW_LEDS];

static void want(uint8_t i, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    if (i >= ROW_LEDS || !a) return;
    req_col[i]  = (col_t){r, g, b};
    req_a[i]    = a;
    req_snap[i] = false;
    req_exact[i] = false;
    req_fast[i] = false;
}

static void want_fast(uint8_t i, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    want(i, r, g, b, a);
    if (i < ROW_LEDS && a) req_fast[i] = true;
}

static void want_now(uint8_t i, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    want(i, r, g, b, a);
    if (i < ROW_LEDS && a) req_snap[i] = true;
}

static void want_exact(uint8_t i, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    want(i, r, g, b, a);
    if (i < ROW_LEDS && a) req_exact[i] = true;
}

static uint16_t approach(uint16_t cur, uint16_t target, uint32_t up, uint32_t down) {
    if (cur < target) return ((uint32_t)(target - cur) > up) ? (uint16_t)(cur + up) : target;
    return ((uint32_t)(cur - target) > down) ? (uint16_t)(cur - down) : target;
}

static uint16_t ease16(uint32_t p) {
    if (p >= 65535) return 65535;
    return (uint16_t)((uint64_t)p * p * (3 * 65535ull - 2 * p) / (65535ull * 65535ull));
}

static uint16_t lerp16(uint16_t a, uint16_t b, uint16_t k) {
    return (uint16_t)(a + ((int64_t)b - a) * k / 65535);
}

static uint16_t isqrt32(uint32_t x) {
    uint32_t r = 0, b = 1UL << 30;
    while (b > x) b >>= 2;
    while (b) {
        if (x >= r + b) {
            x -= r + b;
            r = (r >> 1) + b;
        } else {
            r >>= 1;
        }
        b >>= 2;
    }
    return (uint16_t)r;
}
static uint16_t to_percept(uint16_t light) { return isqrt32((uint32_t)light * 65535); }
static uint16_t to_light(uint16_t p) { return (uint16_t)((uint32_t)p * p / 65535); }

#define XF_MS 400
static bool         xf_seen   = false;
static uint8_t      xf_mode   = 0;
static bool         xf_active = false;
static uint32_t     xf_timer  = 0;
static ws2812_led_t xf_from[ROW_LEDS];

static uint8_t breathe(uint16_t period_ms) {
    uint16_t ph = (uint16_t)((timer_read32() % period_ms) * 256 / period_ms);
    uint16_t p  = (ph * (256 - ph)) >> 6;
    if (p > 255) p = 255;
    return 40 + p * 215 / 255;
}

static uint16_t idle_target(uint32_t e) {
    bool     dim       = cfg.flags & PF_IDLE_DIM;
    uint32_t dim_after = (uint32_t)cfg.idle_dim_s * 1000;
    uint16_t dim_level = dim ? to_percept((uint16_t)((uint32_t)cfg.idle_dim_pct * 65535 / 100)) : 65535;

    if (dim && e < dim_after) return 65535;
    if (dim && e < dim_after + IDLE_DIM_MS) return lerp16(65535, dim_level, ease16((e - dim_after) * 65535 / IDLE_DIM_MS));
    if (e < IDLE_OFF_START || !(cfg.flags & PF_SLEEP_FADE)) return dim_level;
    if (e < IDLE_OFF_START + IDLE_OFF_MS) return lerp16(dim_level, 0, ease16((e - IDLE_OFF_START) * 65535 / IDLE_OFF_MS));
    return 0;
}

enum hud_kinds { HUD_NONE, HUD_VAL, HUD_SPD, HUD_COLOR };
#define HUD_MS 1000
static uint8_t  hud_kind  = HUD_NONE;
static uint32_t hud_timer = 0;

static void hud_show(uint8_t kind) {
    if (!(cfg.flags & PF_HUD)) return;
    hud_kind  = kind;
    hud_timer = timer_read32();
}

static bool hud_active(void) {
    if (hud_kind == HUD_NONE) return false;
    if (timer_elapsed32(hud_timer) > HUD_MS) {
        hud_kind = HUD_NONE;
        return false;
    }
    return true;
}

static void preview_bar(uint32_t level, uint32_t max) {
    uint32_t fill = level * 12 * 256 / max;
    for (uint8_t k = 0; k < 12; k++) {
        uint32_t f = fill > k * 256u ? fill - k * 256u : 0;
        if (f > 255) f = 255;
        if (f < 255) want_fast(LED_F1_INDEX + k, 0, 0, 0, (uint8_t)(255 - f));
    }
}

static void show_nav_echo(void) {
    if (cfg.nav & NAV_NO_ECHO) return;
    for (uint8_t i = 0; i < NAV_KEY_COUNT; i++) {
        if (nav_held & (1 << i)) want_fast(nav_keys[i].echo_led, 120, 120, 120, 255);
    }
}

static void show_hud(void) {
    want_fast(LED_ESC_INDEX, 0, 0, 0, 255);
    want_fast(LED_DEL_INDEX, 0, 0, 0, 255);
    switch (hud_kind) {
        case HUD_VAL: preview_bar(rgb_matrix_get_val(), RGB_MATRIX_MAXIMUM_BRIGHTNESS); break;
        case HUD_SPD: preview_bar(rgb_matrix_get_speed(), 255); break;
        default: break;
    }
}

#define RGB_REPEAT_DELAY 350
#define RGB_REPEAT_EVERY 45
static uint16_t rgb_repeat_kc    = KC_NO;
static uint32_t rgb_repeat_timer = 0;
static bool     rgb_repeated     = false;
static hsv_t    rgb_repeat_hsv;
static uint8_t  rgb_repeat_speed;

static bool rgb_repeat_step(uint16_t kc) {
    switch (kc) {
        case QK_UNDERGLOW_VALUE_UP:
        case QK_RGB_MATRIX_VALUE_UP: rgb_matrix_increase_val_noeeprom(); return true;
        case QK_UNDERGLOW_VALUE_DOWN:
        case QK_RGB_MATRIX_VALUE_DOWN: rgb_matrix_decrease_val_noeeprom(); return true;
        case QK_UNDERGLOW_HUE_UP:
        case QK_RGB_MATRIX_HUE_UP: rgb_matrix_increase_hue_noeeprom(); return true;
        case QK_UNDERGLOW_HUE_DOWN:
        case QK_RGB_MATRIX_HUE_DOWN: rgb_matrix_decrease_hue_noeeprom(); return true;
        case QK_UNDERGLOW_SATURATION_UP:
        case QK_RGB_MATRIX_SATURATION_UP: rgb_matrix_increase_sat_noeeprom(); return true;
        case QK_UNDERGLOW_SATURATION_DOWN:
        case QK_RGB_MATRIX_SATURATION_DOWN: rgb_matrix_decrease_sat_noeeprom(); return true;
        case QK_UNDERGLOW_SPEED_UP:
        case QK_RGB_MATRIX_SPEED_UP: rgb_matrix_increase_speed_noeeprom(); return true;
        case QK_UNDERGLOW_SPEED_DOWN:
        case QK_RGB_MATRIX_SPEED_DOWN: rgb_matrix_decrease_speed_noeeprom(); return true;
    }
    return false;
}

static uint8_t rgb_hud_kind(uint16_t kc) {
    switch (kc) {
        case QK_UNDERGLOW_VALUE_UP: case QK_UNDERGLOW_VALUE_DOWN:
        case QK_RGB_MATRIX_VALUE_UP: case QK_RGB_MATRIX_VALUE_DOWN: return HUD_VAL;
        case QK_UNDERGLOW_SPEED_UP: case QK_UNDERGLOW_SPEED_DOWN:
        case QK_RGB_MATRIX_SPEED_UP: case QK_RGB_MATRIX_SPEED_DOWN: return HUD_SPD;
        case QK_UNDERGLOW_HUE_UP: case QK_UNDERGLOW_HUE_DOWN: case QK_UNDERGLOW_SATURATION_UP: case QK_UNDERGLOW_SATURATION_DOWN:
        case QK_RGB_MATRIX_HUE_UP: case QK_RGB_MATRIX_HUE_DOWN: case QK_RGB_MATRIX_SATURATION_UP: case QK_RGB_MATRIX_SATURATION_DOWN: return HUD_COLOR;
    }
    return HUD_NONE;
}

static void rgb_key_event(uint16_t kc, bool pressed) {
    uint8_t kind = rgb_hud_kind(kc);
    if (kind == HUD_NONE) return;
    if (pressed) {
        hud_show(kind);
        rgb_repeat_kc    = kc;
        rgb_repeat_timer = timer_read32();
        rgb_repeated     = false;
        rgb_repeat_hsv   = (hsv_t){rgb_matrix_get_hue(), rgb_matrix_get_sat(), rgb_matrix_get_val()};
        rgb_repeat_speed = rgb_matrix_get_speed();
    } else if (kc == rgb_repeat_kc) {
        if (rgb_repeated) {
            rgb_matrix_sethsv(rgb_matrix_get_hue(), rgb_matrix_get_sat(), rgb_matrix_get_val());
            rgb_matrix_set_speed(rgb_matrix_get_speed());
        }
        rgb_repeat_kc = KC_NO;
        hud_show(kind);
    }
}

static void rgb_repeat_task(void) {
    if (rgb_repeat_kc == KC_NO) return;
    if (bridge75_mash_active()) {
        if (rgb_repeated) {
            rgb_matrix_sethsv_noeeprom(rgb_repeat_hsv.h, rgb_repeat_hsv.s, rgb_repeat_hsv.v);
            rgb_matrix_set_speed_noeeprom(rgb_repeat_speed);
        }
        rgb_repeat_kc = KC_NO;
        return;
    }
    if (timer_elapsed32(rgb_repeat_timer) < RGB_REPEAT_DELAY) return;
    rgb_repeat_timer = timer_read32() - (RGB_REPEAT_DELAY - RGB_REPEAT_EVERY);
    if (rgb_repeat_step(rgb_repeat_kc)) {
        rgb_repeated = true;
        hud_show(rgb_hud_kind(rgb_repeat_kc));
    }
}

#define SWEEP_MS 380
static bool     sweep_active = false;
static uint32_t sweep_timer  = 0;

static void sweep_start(void) {
    if (!(cfg.flags & PF_SWEEP)) return;
    sweep_active = true;
    sweep_timer  = timer_read32();
}

static void show_sweep(void) {
    if (!sweep_active) return;
    uint32_t t = timer_elapsed32(sweep_timer);
    if (t >= SWEEP_MS) {
        sweep_active = false;
        return;
    }
    rgb_t   c    = hsv_to_rgb((hsv_t){cfg.hue[cfg.profile], cfg.sat[cfg.profile], WHITE_LEVEL});
    int32_t head = -200 + (int32_t)t * (1300 + 400) / SWEEP_MS;
    for (uint8_t i = 0; i < ROW_LEDS; i++) {
        int32_t d = i * 100 - head;
        if (d < 0) d = -d;
        if (d < 200) want_now(i, c.r, c.g, c.b, (uint8_t)((200 - d) * 255 / 200));
    }
}

#define FLICK_AFTER_SWEEP 335
#define FLICK_AFTER_FADE 140
#define FLICK_MS 300

static uint8_t flick_level(uint32_t t) {
    static const struct { uint16_t at, depth; } dips[2] = {{90, 175}, {210, 120}};
    uint8_t lvl = 255;
    for (uint8_t k = 0; k < 2; k++) {
        int32_t d = (int32_t)t - dips[k].at;
        if (d < 0) d = -d;
        if (d >= 55) continue;
        uint32_t f = (uint32_t)(55 - d) * 255 / 55;
        uint32_t s = f * f * (765 - 2 * f) / (255 * 255);
        uint8_t  v = (uint8_t)(255 - s * dips[k].depth / 255);
        if (v < lvl) lvl = v;
    }
    return lvl;
}

static void show_del_flicker(void) {
    if (!del_flicker_active) return;
    uint32_t start = (cfg.flags & PF_SWEEP) ? FLICK_AFTER_SWEEP : FLICK_AFTER_FADE;
    uint32_t e     = timer_elapsed32(del_flicker_timer);
    if (e < start) return;
    if (e >= start + FLICK_MS) {
        del_flicker_active = false;
        return;
    }
    uint8_t lvl = flick_level(e - start);
    uint8_t k   = (uint8_t)((uint16_t)lvl * lvl / 255);
    rgb_t   c   = hsv_to_rgb((hsv_t){cfg.hue[cfg.profile], cfg.sat[cfg.profile], WHITE_LEVEL});
    want_exact(LED_DEL_INDEX, (uint8_t)((uint16_t)c.r * k / 255), (uint8_t)((uint16_t)c.g * k / 255), (uint8_t)((uint16_t)c.b * k / 255), 255);
}

static void show_battery(void) {
#ifdef WIRELESS_ENABLE
    if (gpio_read_pin(BT_CABLE_PIN) && !gpio_read_pin(BT_CHARGE_PIN)) {
        want(LED_ESC_INDEX, WHITE_LEVEL, WHITE_LEVEL, WHITE_LEVEL, 255);
        return;
    }
    uint8_t bat = *md_getp_bat();
    if (bat > 90) {
        want(LED_ESC_INDEX, 0, 200, 0, 255);
    } else if (bat > 50) {
        want(LED_ESC_INDEX, 0, 0, 200, 255);
    } else if (bat > 10) {
        want(LED_ESC_INDEX, 200, 100, 0, 255);
    } else if (bat > 0) {
        want_now(LED_ESC_INDEX, 200, 0, 0, breathe(1600));
    }
#endif
}

#define LED_CONN_FIRST (LED_F1_INDEX + 4)
#define LED_CONN_COUNT 4
#define PULSE_RISE 220
#define PULSE_HOLD 120
#define PULSE_FALL 560
#define PULSE_MS (PULSE_RISE + PULSE_HOLD + PULSE_FALL)

static uint8_t  conn_last_dev   = 0xFF;
static uint8_t  conn_last_state = 0xFF;
static bool     pulse_active    = false;
static uint8_t  pulse_from      = 0;
static uint32_t pulse_timer     = 0;

static uint8_t conn_led(uint8_t dev) {
    switch (dev) {
        case DEVS_BT1: return LED_CONN_FIRST + 1;
        case DEVS_BT2: return LED_CONN_FIRST + 2;
        case DEVS_BT3: return LED_CONN_FIRST + 3;
        default:       return LED_CONN_FIRST;
    }
}

static col_t conn_color(uint8_t dev) {
    switch (dev) {
        case DEVS_USB: return (col_t){200, 80, 0};
        case DEVS_2G4: return (col_t){0, 200, 0};
        default:       return (col_t){0, 0, 200};
    }
}

static uint8_t smooth255(uint32_t u) { return (uint8_t)(u * u * (765 - 2 * u) / (255 * 255)); }

static uint8_t pulse_level(uint32_t t, uint8_t from) {
    uint32_t f;
    if (t < PULSE_RISE) {
        f = from + (uint32_t)(255 - from) * smooth255(t * 255 / PULSE_RISE) / 255;
    } else if (t < PULSE_RISE + PULSE_HOLD) {
        f = 255;
    } else if (t < PULSE_MS) {
        f = 255 - smooth255((t - PULSE_RISE - PULSE_HOLD) * 255 / PULSE_FALL);
    } else {
        return 0;
    }
    return (uint8_t)(f * f / 255);
}

static void show_connectivity(bool fn_held) {
#ifdef WIRELESS_ENABLE
    uint8_t dev   = wireless_get_current_devs();
    uint8_t state = *md_getp_state();
    uint8_t idx   = conn_led(dev);
    col_t   c     = conn_color(dev);

    bool up = (dev == DEVS_USB) ? (conn_last_dev != DEVS_USB && conn_last_dev != 0xFF)
                                : (state == MD_STATE_CONNECTED && (conn_last_state != MD_STATE_CONNECTED || conn_last_dev != dev));
    if (up && (cfg.flags & PF_CONN_PULSE)) {
        pulse_active = true;
        pulse_timer  = timer_read32();
        pulse_from   = mix_a[idx] >> 8;
    }
    conn_last_dev   = dev;
    conn_last_state = state;

    if (fn_held) {
        for (uint8_t d = 0; d < LED_CONN_COUNT; d++) want(LED_CONN_FIRST + d, 20, 20, 20, 255);
    }

    if (dev != DEVS_USB && state == MD_STATE_PAIRING) {
        want_now(idx, c.r, c.g, c.b, breathe(700));
    } else if (dev != DEVS_USB && state != MD_STATE_CONNECTED) {
        want_now(idx, c.r, c.g, c.b, breathe(2000));
    } else if (fn_held) {
        want(idx, c.r, c.g, c.b, 255);
    }

    if (pulse_active) {
        uint32_t t = timer_elapsed32(pulse_timer);
        if (t >= PULSE_MS) {
            pulse_active = false;
        } else if (!fn_held) {
            want_exact(idx, c.r, c.g, c.b, pulse_level(t, pulse_from));
        }
    }
#endif
}

#define LED_F11_INDEX (LED_F1_INDEX + 10)

static void show_keep_awake(bool fn_held) {
    if (fn_held && bridge75_keep_awake()) {
        want(LED_F11_INDEX, 200, 0, 0, 255);
    }
}

static void mix_frame(uint32_t dt) {
    bool fn_held = layer_state & FN_LAYER_MASK;

    memset(req_a, 0, sizeof req_a);
    bool hud = hud_active();
    bool nav = !hud && nav_focus_active();

    if (hud) {
        show_hud();
    } else if (nav) {
        show_nav_echo();
    } else {
        show_connectivity(fn_held);
        show_keep_awake(fn_held);
        if (fn_held) {
            show_battery();
        } else if (host_keyboard_led_state().caps_lock) {
            want_fast(LED_ESC_INDEX, WHITE_LEVEL, WHITE_LEVEL, WHITE_LEVEL, 255);
        }
        if (profile_flash_active && timer_elapsed32(profile_flash_timer) > 2000) {
            profile_flash_active = false;
        }
        if (fn_held || profile_flash_active || cfg.del_mode == DEL_ALWAYS) {
            rgb_t c = hsv_to_rgb((hsv_t){cfg.hue[cfg.profile], cfg.sat[cfg.profile], WHITE_LEVEL});
            want(LED_DEL_INDEX, c.r, c.g, c.b, 255);
        }
        show_sweep();
        show_del_flicker();
    }

    uint32_t up = dt * 65535 / MIX_IN_MS, down = dt * 65535 / MIX_OUT_MS;
    uint32_t fup = dt * 65535 / FAST_IN_MS, fdown = dt * 65535 / FAST_OUT_MS;
    for (uint8_t i = 0; i < ROW_LEDS; i++) {
        uint16_t target = to_percept(req_a[i] * 257);
        if (req_a[i]) {
            mix_col[i]  = req_col[i];
            mix_fast[i] = req_fast[i];
        }
        if (req_exact[i]) {
            mix_a[i] = target;
        } else if (req_snap[i] && target > mix_a[i]) {
            mix_a[i] = target;
        } else if (mix_fast[i]) {
            mix_a[i] = approach(mix_a[i], target, fup, fdown);
        } else {
            mix_a[i] = approach(mix_a[i], target, up, down);
        }
    }
    bool     fx_to   = cfg.fx_on || hud;
    uint16_t fn_dim  = (uint16_t)((uint32_t)cfg.fn_dim_pct * 65535 / 100);
    uint16_t fn_to   = (fn_held && !hud) ? fn_dim : 65535;
    if (nav) fn_to = NAV_FX(cfg.nav) == NAV_FX_NORMAL ? 65535 : (NAV_FX(cfg.nav) == NAV_FX_OFF ? 0 : fn_dim);
    uint32_t down_ms = fx_to ? FN_DIM_IN_MS : FX_FADE_MS;
    eff_gain         = approach(eff_gain, fx_to ? to_percept(fn_to) : 0, dt * 65535 / FN_DIM_OUT_MS, dt * 65535 / down_ms);
    uint32_t eff     = to_light(eff_gain);
    idle_gain        = approach(idle_gain, idle_target(last_input_activity_elapsed()), dt * 65535 / IDLE_WAKE_MS, 65535);
    uint32_t idle    = to_light(idle_gain);

    uint8_t mode = rgb_matrix_get_mode();
    if (xf_seen && mode != xf_mode) {
        memcpy(xf_from, eff_frame, sizeof xf_from);
        xf_active = true;
        xf_timer  = timer_read32();
    }
    xf_seen = true;
    xf_mode = mode;
    uint16_t xf = 65535;
    if (xf_active) {
        uint32_t e = timer_elapsed32(xf_timer);
        if (e >= XF_MS) xf_active = false;
        else xf = ease16(e * 65535 / XF_MS);
    }

    for (uint8_t i = led_lo; i < led_hi && i < ROW_LEDS; i++) {
        ws2812_led_t cur = ws2812_leds[i];
        // effect not always repaint every key, buffer still holding our last write mean keep old color
        if (cur.r != out_last[i].r || cur.g != out_last[i].g || cur.b != out_last[i].b) {
            eff_frame[i] = cur;
        }
        ws2812_led_t e = eff_frame[i];
        if (xf_active) {
            e.r = (uint8_t)lerp16(xf_from[i].r, e.r, xf);
            e.g = (uint8_t)lerp16(xf_from[i].g, e.g, xf);
            e.b = (uint8_t)lerp16(xf_from[i].b, e.b, xf);
        }
        uint32_t a  = to_light(mix_a[i]);
        uint32_t er = (uint32_t)e.r * eff / 65535;
        uint32_t eg = (uint32_t)e.g * eff / 65535;
        uint32_t eb = (uint32_t)e.b * eff / 65535;
        uint32_t r  = (er * (65535 - a) + (uint32_t)mix_col[i].r * a) / 65535;
        uint32_t g  = (eg * (65535 - a) + (uint32_t)mix_col[i].g * a) / 65535;
        uint32_t b  = (eb * (65535 - a) + (uint32_t)mix_col[i].b * a) / 65535;
        r           = r * idle / 65535;
        g           = g * idle / 65535;
        b           = b * idle / 65535;
        rgb_matrix_set_color(i, r, g, b);
        out_last[i] = ws2812_leds[i];
    }
}

bool rgb_matrix_indicators_advanced_user(uint8_t led_min, uint8_t led_max) {
    led_lo = led_min;
    led_hi = led_max;

    uint32_t now = timer_read32();
    uint32_t dt  = mix_last ? now - mix_last : 0;
    if (dt > 40) dt = 40; // after timeout or sleep the fades still run full length
    mix_last = now;

    if (anim_active) {
        rgb_t frame[ANIM_LEDS];
        if (anim_render(timer_elapsed32(anim_timer), frame)) {
            for (uint8_t i = led_lo; i < led_hi && i < ANIM_LEDS; i++) {
                rgb_matrix_set_color(i, frame[i].r, frame[i].g, frame[i].b);
            }
            return false;
        }
        anim_active  = false;
        fade_pending = fade_armed;
    }

    mix_frame(dt);
    return false;
}

bool rgb_matrix_indicators_user(void) {
    return false;
}

static bool suspend_faded = false;

void bridge75_suspend_fade(void) {
    if (suspend_faded) return;
    suspend_faded = true;
    if (!(cfg.flags & PF_SLEEP_FADE)) return;
    ws2812_led_t snap[ROW_LEDS];
    memcpy(snap, ws2812_leds, sizeof snap);
    for (uint8_t step = 1; step <= 20; step++) {
        uint16_t k = to_light(65535 - ease16((uint32_t)step * 65535 / 20));
        for (uint8_t i = 0; i < ROW_LEDS; i++) {
            ws2812_set_color(i, (uint32_t)snap[i].r * k / 65535, (uint32_t)snap[i].g * k / 65535, (uint32_t)snap[i].b * k / 65535);
        }
        ws2812_flush();
        wait_ms(30);
    }
}

void suspend_wakeup_init_user(void) {
    // usb restart when switch to wired land here too, only fade back after real sleep
    if (!suspend_faded) return;
    suspend_faded = false;
    idle_gain     = 0;
    mix_last      = 0;
}

_Static_assert(RGB_MATRIX_CYCLE_ALL == 2 && RGB_MATRIX_RAINBOW_BEACON == 6 && RGB_MATRIX_CUSTOM_BAR_AURORA == 7 &&
                   RGB_MATRIX_CUSTOM_BAR_GLOW == 15 && RGB_MATRIX_EFFECT_MAX == 16,
               "effect numbers changed: update the VIA effect list");

#ifdef RGB_MATRIX_DEFAULT_SPD
_Static_assert(RGB_MATRIX_DEFAULT_MODE == RGB_MATRIX_CUSTOM_BAR_OCEAN && RGB_MATRIX_DEFAULT_SPD == JSCIO_SPEED,
               "keymap config.h defaults and the rev 3 defaults must match");
#endif

static uint8_t rev1_effect(uint8_t m) {
    static const uint8_t old_effect[] = {
        RGB_MATRIX_NONE,               RGB_MATRIX_SOLID_COLOR,        RGB_MATRIX_CUSTOM_BAR_BREATH,
        RGB_MATRIX_CYCLE_ALL,          RGB_MATRIX_CYCLE_LEFT_RIGHT,   RGB_MATRIX_CYCLE_ALL,
        RGB_MATRIX_CYCLE_LEFT_RIGHT,   RGB_MATRIX_CYCLE_OUT_IN,       RGB_MATRIX_CYCLE_OUT_IN,
        RGB_MATRIX_CYCLE_ALL,          RGB_MATRIX_CYCLE_LEFT_RIGHT,   RGB_MATRIX_DUAL_BEACON,
        RGB_MATRIX_RAINBOW_BEACON,     RGB_MATRIX_CUSTOM_BAR_TWINKLE, RGB_MATRIX_CUSTOM_BAR_GLOW,
        RGB_MATRIX_CUSTOM_BAR_RIPPLES, RGB_MATRIX_CUSTOM_BAR_RIPPLES, RGB_MATRIX_CUSTOM_BAR_RIPPLES,
    };
    return m < sizeof old_effect ? old_effect[m] : RGB_MATRIX_CYCLE_LEFT_RIGHT;
}

static void cfg_migrate(void) {
    if (cfg.rev >= CFG_REV) return;
    if (cfg.rev < 1) {
        rgb_matrix_mode(rev1_effect(rgb_matrix_get_mode()));
        for (uint8_t l = PUB_FN; l <= GAM_FN; l++) {
            if (dynamic_keymap_get_keycode(l, 1, 0) == BT_USB) dynamic_keymap_set_keycode(l, 1, 0, BT_SW);
        }
    }
    if (cfg.rev < 2) {
        static const uint16_t old_row[9] = {RGB_TOG, RGB_MOD, RGB_RMOD, RGB_HUI, RGB_HUD, RGB_SAI, RGB_SAD, RGB_VAI, RGB_VAD};
        static const uint16_t new_row[9] = {RGB_RMOD, RGB_MOD, RGB_HUD, RGB_HUI, RGB_SPD, RGB_SPI, KC_TRNS, KC_TRNS, KC_TRNS};
        for (uint8_t l = PUB_FN; l <= GAM_FN; l++) {
            bool stock = true;
            for (uint8_t c = 0; c < 9; c++) {
                if (dynamic_keymap_get_keycode(l, 4, c + 1) != old_row[c]) stock = false;
            }
            if (!stock) continue;
            for (uint8_t c = 0; c < 9; c++) dynamic_keymap_set_keycode(l, 4, c + 1, new_row[c]);
        }
    }
    if (cfg.rev < 3) {
        rgb_matrix_mode(RGB_MATRIX_CUSTOM_BAR_OCEAN);
        rgb_matrix_set_speed(JSCIO_SPEED);
        cfg.startup_anim = ANIM_WELCOME;
        cfg.anim_color   = ANIM_COLOR_CUSTOM;
        cfg.anim_hue     = cfg_defaults.anim_hue;
        cfg.anim_sat     = cfg_defaults.anim_sat;
    }
    cfg.rev = CFG_REV;
    cfg_save();
}

void keyboard_post_init_user(void) {
    if (eeconfig_is_user_datablock_valid()) {
        eeconfig_read_user_datablock(&cfg);
        cfg_sanitize();
    } else if (eeprom_read_dword(EECONFIG_USER) == PERSONAL_CONFIG_V1_SIZE) {
        cfg = cfg_defaults;
        eeprom_read_block(&cfg, EECONFIG_USER_DATABLOCK, PERSONAL_CONFIG_V1_SIZE);
        cfg_sanitize();
        cfg_save();
    } else {
        cfg = cfg_defaults;
        cfg_save();
    }

    cfg_migrate();
    cfg_apply();
    eff_gain = cfg.fx_on ? 65535 : 0;

    if (bridge75_cold_boot) {
        startup_play(cfg.startup_anim);
    }
}

enum via_rj_values {
    id_profile      = 1,
    id_del_mode     = 2,
    id_socd_mode    = 3,
    id_startup_anim = 4,
    id_fx_on        = 5,
    id_rgb_off_min  = 6,
    id_color_pub    = 7,
    id_color_per    = 8,
    id_color_gam    = 9,
    id_anim_color   = 10,
    id_anim_custom  = 11,
    id_anim_play    = 12,
    id_idle_dim     = 13,
    id_idle_dim_pct = 14,
    id_idle_dim_s   = 15,
    id_hud          = 16,
    id_sweep        = 17,
    id_conn_pulse   = 18,
    id_sleep_fade   = 19,
    id_wake_replay  = 20,
    id_fn_dim_pct   = 21,
    id_nav_focus    = 22,
    id_nav_fx       = 23,
    id_nav_echo     = 24,
};

static uint8_t via_flag_bit(uint8_t id) {
    switch (id) {
        case id_idle_dim:    return PF_IDLE_DIM;
        case id_hud:         return PF_HUD;
        case id_sweep:       return PF_SWEEP;
        case id_conn_pulse:  return PF_CONN_PULSE;
        case id_sleep_fade:  return PF_SLEEP_FADE;
        case id_wake_replay: return PF_WAKE_REPLAY;
    }
    return 0;
}

static void via_get(uint8_t id, uint8_t *v) {
    switch (id) {
        case id_profile:      v[0] = cfg.profile; break;
        case id_del_mode:     v[0] = cfg.del_mode; break;
        case id_socd_mode:    v[0] = cfg.socd_mode; break;
        case id_startup_anim: v[0] = cfg.startup_anim; break;
        case id_anim_color:   v[0] = cfg.anim_color; break;
        case id_idle_dim_pct: v[0] = cfg.idle_dim_pct; break;
        case id_idle_dim_s:   v[0] = cfg.idle_dim_s; break;
        case id_fn_dim_pct:   v[0] = cfg.fn_dim_pct; break;
        case id_nav_focus:    v[0] = !(cfg.nav & NAV_OFF); break;
        case id_nav_fx:       v[0] = NAV_FX(cfg.nav); break;
        case id_nav_echo:     v[0] = !(cfg.nav & NAV_NO_ECHO); break;
        case id_idle_dim:
        case id_hud:
        case id_sweep:
        case id_conn_pulse:
        case id_sleep_fade:
        case id_wake_replay:  v[0] = (cfg.flags & via_flag_bit(id)) ? 1 : 0; break;
        case id_anim_custom:
            v[0] = cfg.anim_hue;
            v[1] = cfg.anim_sat;
            break;
        case id_fx_on:        v[0] = cfg.fx_on; break;
        case id_rgb_off_min:  v[0] = cfg.rgb_off_min; break;
        case id_color_pub:
        case id_color_per:
        case id_color_gam:
            v[0] = cfg.hue[id - id_color_pub];
            v[1] = cfg.sat[id - id_color_pub];
            break;
    }
}

static void via_set(uint8_t id, uint8_t *v) {
    switch (id) {
        case id_profile:
            if (v[0] < PROFILE_COUNT) profile_apply(v[0], true);
            break;
        case id_del_mode:
            if (v[0] < DEL_MODE_COUNT) cfg.del_mode = v[0];
            break;
        case id_socd_mode:
            if (v[0] < SOCD_MODE_COUNT) {
                socd_release_all();
                cfg.socd_mode = v[0];
            }
            break;
        case id_startup_anim:
            if (v[0] < ANIM_COUNT) cfg.startup_anim = v[0];
            break;
        case id_anim_color:
            if (v[0] < ANIM_COLOR_COUNT) cfg.anim_color = v[0];
            break;
        case id_anim_custom:
            cfg.anim_hue = v[0];
            cfg.anim_sat = v[1];
            break;
        case id_anim_play:
            startup_play(cfg.startup_anim == ANIM_OFF ? ANIM_WELCOME : cfg.startup_anim);
            break;
        case id_idle_dim_pct:
            if (v[0] >= 5 && v[0] <= 95) cfg.idle_dim_pct = v[0];
            break;
        case id_idle_dim_s:
            if (v[0] >= IDLE_DIM_S_MIN && v[0] <= IDLE_DIM_S_MAX) cfg.idle_dim_s = v[0];
            break;
        case id_fn_dim_pct:
            if (v[0] <= 100) cfg.fn_dim_pct = v[0];
            break;
        case id_nav_focus:
            if (v[0]) cfg.nav &= ~NAV_OFF;
            else cfg.nav |= NAV_OFF;
            break;
        case id_nav_fx:
            if (v[0] < NAV_FX_COUNT) cfg.nav = (cfg.nav & ~0x0C) | (v[0] << 2);
            break;
        case id_nav_echo:
            if (v[0]) cfg.nav &= ~NAV_NO_ECHO;
            else cfg.nav |= NAV_NO_ECHO;
            break;
        case id_idle_dim:
        case id_hud:
        case id_sweep:
        case id_conn_pulse:
        case id_sleep_fade:
        case id_wake_replay:
            if (v[0]) cfg.flags |= via_flag_bit(id);
            else cfg.flags &= ~via_flag_bit(id);
            cfg_apply();
            break;
        case id_fx_on:        cfg.fx_on = !!v[0]; break;
        case id_rgb_off_min:
            cfg.rgb_off_min               = v[0];
            bridge75_rgb_hard_timeout_min = v[0];
            break;
        case id_color_pub:
        case id_color_per:
        case id_color_gam:
            cfg.hue[id - id_color_pub] = v[0];
            cfg.sat[id - id_color_pub] = v[1];
            profile_flash_timer        = timer_read32();
            profile_flash_active       = true;
            break;
    }
}

void via_custom_value_command_kb(uint8_t *data, uint8_t length) {
    if (data[1] != id_custom_channel) {
        data[0] = id_unhandled;
        return;
    }
    switch (data[0]) {
        case id_custom_set_value:
            via_set(data[2], &data[3]);
            break;
        case id_custom_get_value:
            via_get(data[2], &data[3]);
            break;
        case id_custom_save:
            cfg_save();
            break;
        default:
            data[0] = id_unhandled;
            break;
    }
}
