#include "qmk_stub.h"
#include "bridge75.h"
#include "color.h"
#include "via.h"
#include "wireless.h"
#include "ws2812.h"
#include <stdio.h>
#include <stdlib.h>

layer_state_t layer_state = 0, default_layer_state = 1;
bool          bridge75_cold_boot            = false;
uint16_t      bridge75_rgb_hard_timeout_min = 0;

static uint32_t now_ms;
static uint8_t  ee_data[EECONFIG_USER_DATA_SIZE];
static bool     ee_valid;
static int      ee_writes;
ws2812_led_t ws2812_leds[14];
static bool     caps;
static bool     cable, charging;
static uint8_t  md_state = MD_STATE_CONNECTED, md_bat = 95, cur_dev = DEVS_USB;
static char     events[512];

void     default_layer_set(layer_state_t s) { default_layer_state = s; }
uint32_t timer_read32(void) { return now_ms; }
uint32_t timer_elapsed32(uint32_t last) { return now_ms - last; }
static void ev(const char *s) { strcat(events, s); }
static bool down[256];
static int  socd_violations;
static uint8_t opposite(uint8_t kc) {
    switch (kc) { case KC_A: return KC_D; case KC_D: return KC_A; case KC_W: return KC_S; case KC_S: return KC_W; }
    return 0;
}
void register_code(uint8_t kc) {
    down[kc] = true;
    if (opposite(kc) && down[opposite(kc)]) socd_violations++;
    char b[8]; snprintf(b, sizeof b, "+%c ", 'A' + kc - KC_A); ev(b); }
void unregister_code(uint8_t kc) { down[kc] = false; char b[8]; snprintf(b, sizeof b, "-%c ", 'A' + kc - KC_A); ev(b); }
void rgb_matrix_set_color(int i, uint8_t r, uint8_t g, uint8_t b) {
    if (i < 0 || i >= 14) { printf("FAIL: LED index %d out of range\n", i); exit(1); }
    ws2812_leds[i] = (ws2812_led_t){r, g, b};
}
led_t host_keyboard_led_state(void) { led_t l = {0}; l.caps_lock = caps; return l; }
bool  gpio_read_pin(int pin) { return pin == BT_CABLE_PIN ? cable : !charging; }
static uint32_t ee_version;
static uint8_t  ee_raw[64];
bool  eeconfig_is_user_datablock_valid(void) { return ee_valid; }
uint32_t eeprom_read_dword(const uint32_t *addr) { (void)addr; return ee_version; }
void     eeprom_read_block(void *buf, const void *addr, uint32_t len) { (void)addr; memcpy(buf, ee_raw, len); }
void  eeconfig_read_user_datablock(void *d) { memcpy(d, ee_data, sizeof ee_data); }
void  eeconfig_update_user_datablock(const void *d) { memcpy(ee_data, d, sizeof ee_data); memcpy(ee_raw, d, sizeof ee_data); ee_valid = true; ee_version = EECONFIG_USER_DATA_SIZE; ee_writes++; }
static bool hsv_identity = true;
rgb_t hsv_to_rgb(hsv_t h) {
    if (hsv_identity) { rgb_t c = {h.h, h.s, h.v}; return c; }
    rgb_t c;
    if (h.s == 0) { c.r = c.g = c.b = h.v; return c; }
    uint8_t region = h.h * 6 / 255, rem = (h.h * 2 - region * 85) * 3;
    uint8_t p = (h.v * (255 - h.s)) >> 8, q = (h.v * (255 - ((h.s * rem) >> 8))) >> 8, t = (h.v * (255 - ((h.s * (255 - rem)) >> 8))) >> 8;
    switch (region) {
        case 6: case 0: c = (rgb_t){h.v, t, p}; break;
        case 1: c = (rgb_t){q, h.v, p}; break;
        case 2: c = (rgb_t){p, h.v, t}; break;
        case 3: c = (rgb_t){p, q, h.v}; break;
        case 4: c = (rgb_t){t, p, h.v}; break;
        default: c = (rgb_t){h.v, p, q}; break;
    }
    return c;
}

static bool    rgbm_enabled = true;
static uint8_t rgbm_h = 21, rgbm_s = 255, rgbm_v = 150;
static int     rgbm_sets;
bool    rgb_matrix_is_enabled(void) { return rgbm_enabled; }
uint8_t rgb_matrix_get_val(void) { return rgbm_v; }
uint8_t rgb_matrix_get_hue(void) { return rgbm_h; }
uint8_t rgb_matrix_get_sat(void) { return rgbm_s; }
void    rgb_matrix_sethsv_noeeprom(uint16_t h, uint8_t s, uint8_t v) {
    if (!rgbm_enabled) return;
    rgbm_h = h; rgbm_s = s; rgbm_v = v > 200 ? 200 : v; rgbm_sets++;
}
uint8_t *md_getp_state(void) { return &md_state; }
uint8_t *md_getp_bat(void) { return &md_bat; }
uint8_t  wireless_get_current_devs(void) { return cur_dev; }
static bool keep_awake;
bool bridge75_keep_awake(void) { return keep_awake; }
bool bridge75_wake_replay = true;
static bool mash_active;
bool bridge75_mash_active(void) { return mash_active; }

static uint32_t idle_ms;
static uint8_t  rgbm_speed = 128;
static int      rgbm_eeprom_saves, rgbm_steps, flushes;
static uint8_t  flush_trace[32];
uint32_t last_input_activity_elapsed(void) { return idle_ms; }
void     wait_ms(uint32_t ms) { now_ms += ms; }
uint8_t  rgb_matrix_get_speed(void) { return rgbm_speed; }
void     rgb_matrix_sethsv(uint16_t h, uint8_t s, uint8_t v) { rgb_matrix_sethsv_noeeprom(h, s, v); rgbm_eeprom_saves++; }
void     rgb_matrix_set_speed(uint8_t sp) { rgbm_speed = sp; }
void     rgb_matrix_set_speed_noeeprom(uint8_t sp) { rgbm_speed = sp; }
void     rgb_matrix_increase_val_noeeprom(void) { rgbm_v = rgbm_v + 8 > 200 ? 200 : rgbm_v + 8; rgbm_steps++; }
void     rgb_matrix_decrease_val_noeeprom(void) { rgbm_v = rgbm_v < 8 ? 0 : rgbm_v - 8; rgbm_steps++; }
void     rgb_matrix_increase_hue_noeeprom(void) { rgbm_h += 8; rgbm_steps++; }
void     rgb_matrix_decrease_hue_noeeprom(void) { rgbm_h -= 8; rgbm_steps++; }
void     rgb_matrix_increase_sat_noeeprom(void) { rgbm_steps++; }
void     rgb_matrix_decrease_sat_noeeprom(void) { rgbm_steps++; }
void     rgb_matrix_increase_speed_noeeprom(void) { rgbm_speed += 16; rgbm_steps++; }
void     rgb_matrix_decrease_speed_noeeprom(void) { rgbm_speed -= 16; rgbm_steps++; }
void     ws2812_set_color(int i, uint8_t r, uint8_t g, uint8_t b) { ws2812_leds[i] = (ws2812_led_t){r, g, b}; }
void     ws2812_flush(void) { if (flushes < 32) flush_trace[flushes] = ws2812_leds[0].r; flushes++; }

static uint8_t  rgbm_mode = RGB_MATRIX_CYCLE_LEFT_RIGHT;
static int      rgbm_mode_saves;
uint8_t rgb_matrix_get_mode(void) { return rgbm_mode; }
void    rgb_matrix_mode(uint8_t m) { rgbm_mode = m; rgbm_mode_saves++; }
static uint16_t dkm[6][6][15];
uint16_t dynamic_keymap_get_keycode(uint8_t l, uint8_t r, uint8_t c) { return dkm[l][r][c]; }
void     dynamic_keymap_set_keycode(uint8_t l, uint8_t r, uint8_t c, uint16_t kc) { dkm[l][r][c] = kc; }
#include "../../keyboards/shortcut/bridge75/ansi_min/keymaps/rj/keymap.c"

static int failures, checks;
#define CHECK(cond, ...) do { checks++; if (!(cond)) { failures++; printf("FAIL %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

static bool key(uint16_t kc, bool pressed) {
    keyrecord_t r = {.event = {.pressed = pressed}};
    return process_record_user(kc, &r);
}
static void clear_events(void) { events[0] = 0; }
static uint8_t eff_col = 0x55;
static void frame(void) {
    memset(ws2812_leds, eff_col, sizeof ws2812_leds);
    rgb_matrix_indicators_advanced_user(0, 14);
}
static void settle_to(uint32_t t) {
    while ((int32_t)(t - now_ms) > 0) {
        now_ms += 16;
        frame();
    }
}
static void settle(void) { settle_to(now_ms + 480); }
static int absd(int a, int b) { return a > b ? a - b : b - a; }

static uint16_t resolve(int idx) {
    layer_state_t all = layer_state | default_layer_state;
    for (int l = GAM_FN; l >= 0; l--) {
        if (!(all & (1UL << l))) continue;
        uint16_t kc = ((const uint16_t *)keymaps[l])[idx];
        if (kc != KC_TRNS) return kc;
    }
    return KC_NO;
}
static bool led_is(int i, int r, int g, int b) { return ws2812_leds[i].r == r && ws2812_leds[i].g == g && ws2812_leds[i].b == b; }
static void boot(bool cold) {
    bridge75_cold_boot = cold;
    layer_state        = 0;
    keyboard_post_init_user();
}

int main(void) {
    ee_valid = false;
    boot(false);
    CHECK(ee_valid && ee_writes == 1, "defaults not saved on first boot");
    CHECK(cfg.profile == PROFILE_PUBLIC && default_layer_state == (1UL << PUB_B), "default profile wrong");
    CHECK(bridge75_rgb_hard_timeout_min == 30, "hard RGB timeout not applied");

    CHECK(!key(PRF_NXT, true) && !key(PRF_NXT, false), "PRF_NXT must be consumed");
    CHECK(cfg.profile == PROFILE_PERSONAL && default_layer_state == (1UL << PER_B), "next -> Personal");
    key(PRF_NXT, true); key(PRF_NXT, false);
    CHECK(cfg.profile == PROFILE_GAMING && default_layer_state == (1UL << GAM_B), "next -> Gaming");
    key(PRF_NXT, true); key(PRF_NXT, false);
    CHECK(cfg.profile == PROFILE_PUBLIC && default_layer_state == (1UL << PUB_B), "next wraps to Public");
    CHECK(((rj_config_t *)ee_data)->profile == PROFILE_PUBLIC, "profile not persisted");

    key(PRF_GAM, true); key(PRF_GAM, false);
    boot(false);
    CHECK(cfg.profile == PROFILE_GAMING && default_layer_state == (1UL << GAM_B), "profile lost after reboot");

#define K(layer, idx) (((const uint16_t *)keymaps[layer])[idx])
    enum { I_ESC = 0, I_F1 = 1, I_DEL = 13, I_HOME = 28, I_CAPS = 44, I_H = 50, I_J = 51, I_K = 52, I_L = 53,
           I_END = 71, I_LCTL = 72, I_FN = 77, I_RGHT = 80 };
    CHECK(K(PUB_B, I_RGHT) == KC_RGHT && K(PUB_B, I_DEL) == KC_DEL && K(PUB_B, I_HOME) == KC_HOME, "layout index sanity");
    CHECK(K(PER_B, I_CAPS) == KC_LCTL && K(PER_B, I_LCTL) == KC_CAPS, "Personal Caps/Ctrl swap");
    CHECK(K(GAM_B, I_CAPS) == KC_LCTL && K(GAM_B, I_LCTL) == KC_CAPS, "Gaming Caps/Ctrl swap");
    CHECK(K(PUB_B, I_CAPS) == KC_CAPS && K(PUB_B, I_LCTL) == KC_LCTL, "Public keeps Caps/Ctrl");
    CHECK(K(PER_FN, I_H) == KC_LEFT && K(PER_FN, I_J) == KC_DOWN && K(PER_FN, I_K) == KC_UP && K(PER_FN, I_L) == KC_RGHT, "Personal FN+HJKL");
    CHECK(K(GAM_FN, I_H) == KC_LEFT && K(GAM_FN, I_J) == KC_DOWN && K(GAM_FN, I_K) == KC_UP && K(GAM_FN, I_L) == KC_RGHT, "Gaming FN+HJKL");
    CHECK(K(PUB_FN, I_H) == KC_TRNS, "Public FN+H stays H");
    for (int p = 0; p < 3; p++) {
        int fn = PUB_FN + p, base = PUB_B + p;
        const uint16_t want[13] = {KC_BRID, KC_BRIU, KC_VOLD, KC_VOLU, KC_MUTE, KC_MPRV, KC_MPLY, KC_MNXT,
                                   RGB_VAD, RGB_VAI, FX_TOG, KC_PSCR, KC_INS};
        for (int i = 0; i < 13; i++) CHECK(K(fn, I_F1 + i) == want[i], "profile %d FN key %d", p, I_F1 + i);
        CHECK(K(fn, I_ESC) == KC_TRNS, "FN+Esc is Esc (no EEPROM wipe) profile %d", p);
        CHECK(K(fn, I_HOME) == PRF_NXT, "FN+Home profile %d", p);
        CHECK(K(fn, I_END) == QK_REBOOT, "FN+End profile %d", p);
        CHECK(K(base, I_FN) == MO(fn), "FN key of profile %d points at its own FN layer", p);
        CHECK(K(fn, I_FN) == KC_TRNS, "FN key transparent on FN layer %d", p);
    }

    key(PRF_PUB, true); key(PRF_PUB, false);
    layer_state = 1UL << PUB_FN;
    CHECK(resolve(I_HOME) == PRF_NXT, "FN+Home resolves to PRF_NXT");
    key(PRF_NXT, true); key(PRF_NXT, false);
    CHECK(cfg.profile == PROFILE_PERSONAL, "-> Personal");
    CHECK(resolve(I_HOME) == PRF_NXT, "FN still active after switch (FN layer not hidden by new base)");
    key(PRF_NXT, true); key(PRF_NXT, false);
    CHECK(cfg.profile == PROFILE_GAMING, "second FN+Home while FN held -> Gaming");
    CHECK(resolve(I_HOME) == PRF_NXT && resolve(I_F1) == KC_BRID, "FN combos keep working");
    layer_state = 0;
    CHECK(resolve(I_HOME) == KC_HOME && resolve(I_CAPS) == KC_LCTL, "Gaming base after FN release");
    layer_state = 1UL << GAM_FN;
    CHECK(resolve(I_H) == KC_LEFT, "re-pressed FN uses Gaming FN layer");
    layer_state = 0;

    cfg.socd_mode = SOCD_LAST;
    clear_events();
    CHECK(!key(KC_A, true), "SOCD must consume A");
    key(KC_D, true);
    key(KC_D, false);
    key(KC_A, false);
    CHECK(!strcmp(events, "+A -A +D -D +A -A "), "last-wins sequence: %s", events);

    cfg.socd_mode = SOCD_NEUTRAL;
    clear_events();
    key(KC_W, true); key(KC_S, true); key(KC_S, false); key(KC_W, false);
    CHECK(!strcmp(events, "+W -W +W -W "), "neutral sequence: %s", events);

    cfg.socd_mode = SOCD_FIRST;
    clear_events();
    key(KC_A, true); key(KC_D, true); key(KC_A, false); key(KC_D, false);
    CHECK(!strcmp(events, "+A -A +D -D "), "first-wins sequence: %s", events);

    clear_events();
    key(KC_A, true); key(KC_D, true); key(KC_A, false); key(KC_A, true);
    CHECK(!strcmp(events, "+A -A +D "), "first-wins after re-press keeps D: %s", events);
    key(KC_A, false); key(KC_D, false);

    cfg.socd_mode = SOCD_LAST;
    clear_events();
    key(KC_W, true); key(KC_D, true);
    CHECK(!strcmp(events, "+W +D "), "diagonal must keep both axes: %s", events);
    key(KC_W, false); key(KC_D, false);
    CHECK(key(KC_E, true) && key(KC_E, false), "non-WASD keys must pass to QMK");

    CHECK(socd_violations == 0, "host saw opposite directions held together %d times", socd_violations);

    cfg.socd_mode = SOCD_OFF;
    CHECK(key(KC_A, true) && key(KC_A, false), "SOCD off must pass through");
    cfg.socd_mode = SOCD_LAST;
    key(PRF_PER, true); key(PRF_PER, false);
    CHECK(key(KC_A, true) && key(KC_A, false), "Personal profile must not apply SOCD");

    CHECK(key(KC_W, true), "W press in Personal goes to QMK");
    key(PRF_GAM, true); key(PRF_GAM, false);
    clear_events();
    CHECK(key(KC_W, false), "release must reach QMK or W sticks");
    CHECK(events[0] == 0, "no synthetic events expected: %s", events);

    clear_events();
    key(KC_A, true);
    key(PRF_PUB, true); key(PRF_PUB, false);
    CHECK(!strcmp(events, "+A -A "), "switching away must release SOCD keys: %s", events);
    CHECK(key(KC_A, false), "late release passes through harmlessly");

    uint8_t msg[32];
    memset(msg, 0, sizeof msg);
    msg[0] = id_custom_set_value; msg[1] = 0; msg[2] = id_profile; msg[3] = PROFILE_PERSONAL;
    via_custom_value_command_kb(msg, sizeof msg);
    CHECK(cfg.profile == PROFILE_PERSONAL && default_layer_state == (1UL << PER_B), "VIA set profile");
    msg[0] = id_custom_get_value; msg[2] = id_profile; msg[3] = 0xEE;
    via_custom_value_command_kb(msg, sizeof msg);
    CHECK(msg[3] == PROFILE_PERSONAL && msg[0] == id_custom_get_value, "VIA get profile");
    msg[0] = id_custom_set_value; msg[2] = id_socd_mode; msg[3] = 9;
    via_custom_value_command_kb(msg, sizeof msg);
    CHECK(cfg.socd_mode == SOCD_LAST, "VIA rejects out-of-range SOCD mode");
    msg[0] = id_custom_set_value; msg[2] = id_color_gam; msg[3] = 85; msg[4] = 128;
    via_custom_value_command_kb(msg, sizeof msg);
    CHECK(cfg.hue[2] == 85 && cfg.sat[2] == 128, "VIA set colour");
    msg[0] = id_custom_get_value; msg[2] = id_color_gam; msg[3] = msg[4] = 0;
    via_custom_value_command_kb(msg, sizeof msg);
    CHECK(msg[3] == 85 && msg[4] == 128, "VIA get colour");
    int w = ee_writes;
    msg[0] = id_custom_save;
    via_custom_value_command_kb(msg, sizeof msg);
    CHECK(ee_writes == w + 1 && ((rj_config_t *)ee_data)->hue[2] == 85, "VIA save persists");
    msg[0] = id_custom_set_value; msg[1] = 3;
    via_custom_value_command_kb(msg, sizeof msg);
    CHECK(msg[0] == id_unhandled, "foreign channel must be unhandled");
    msg[0] = 0x42; msg[1] = 0;
    via_custom_value_command_kb(msg, sizeof msg);
    CHECK(msg[0] == id_unhandled, "unknown command must be unhandled");
    msg[0] = id_custom_set_value; msg[1] = 0; msg[2] = id_rgb_off_min; msg[3] = 0;
    via_custom_value_command_kb(msg, sizeof msg);
    CHECK(bridge75_rgb_hard_timeout_min == 0, "VIA hard RGB timeout applied live");

    now_ms += 5000; settle();
    CHECK(led_is(0, 0x55, 0x55, 0x55) && led_is(13, 0x55, 0x55, 0x55), "idle: effect untouched");
    caps = true; now_ms += 16; frame();
    CHECK(!led_is(0, 200, 200, 200) && !led_is(0, 0x55, 0x55, 0x55), "caps lock fades in (not instant)");
    settle();
    CHECK(led_is(0, 200, 200, 200), "caps lock -> white Esc");
    caps = false; now_ms += 16; frame();
    CHECK(!led_is(0, 0x55, 0x55, 0x55), "caps lock fades out (not instant)");
    settle();
    CHECK(led_is(0, 0x55, 0x55, 0x55), "caps off -> effect back");
    caps = true; settle();
    layer_state = 1UL << PER_FN;
    md_bat = 95;
    settle();
    CHECK(led_is(0, 0, 200, 0), "FN: battery >90 green (beats caps)");
    CHECK(led_is(13, cfg.hue[1], cfg.sat[1], 200), "FN: profile colour on Delete");
    CHECK(led_is(5, 200, 80, 0) && led_is(6, 20, 20, 20) && led_is(8, 20, 20, 20), "FN: USB cable orange on F5, F6-F8 dim");
    CHECK(absd(ws2812_leds[2].r, 0x55 * 25 / 100) <= 1 && absd(ws2812_leds[9].g, 0x55 * 25 / 100) <= 1,
          "FN: effect dims to 25%% so indicators stand out (got %u)", ws2812_leds[2].r);
    md_bat = 30; settle();
    CHECK(led_is(0, 200, 100, 0), "battery 11-50 yellow");
    cable = true; charging = true; settle();
    CHECK(led_is(0, 200, 200, 200), "charging -> white");
    cable = charging = false;
    cur_dev = DEVS_BT2; md_state = MD_STATE_CONNECTED; settle();
    CHECK(led_is(7, 0, 0, 200) && led_is(5, 20, 20, 20), "FN: BT2 connected blue on F7");
    cur_dev = DEVS_2G4; settle();
    CHECK(led_is(5, 0, 200, 0), "FN: 2.4G dongle green on F5");
    layer_state = 0; caps = false;
    now_ms += 16; frame();
    CHECK(ws2812_leds[2].r < 0x55 && ws2812_leds[2].r > 0x55 * 25 / 100, "FN released: effect eases back (got %u)", ws2812_leds[2].r);
    settle();
    CHECK(ws2812_leds[2].r == 0x55, "FN released: effect back to full");
    cur_dev = DEVS_BT1; md_state = MD_STATE_DISCONNECTED;
    settle_to(200000 + 1000); uint8_t peak = ws2812_leds[6].b;
    settle_to(202000);        uint8_t low  = ws2812_leds[6].b;
    CHECK(peak >= 195 && low < peak - 50 && low > 0x55, "disconnected BT1 breathes on F6 (peak %u, low %u), never fully dark", peak, low);
    md_state = MD_STATE_CONNECTED; cur_dev = DEVS_USB;
    cfg.fx_on = 0; now_ms += 16; frame();
    CHECK(ws2812_leds[6].r > 0, "FX off fades (not instant)");
    settle();
    CHECK(led_is(6, 0, 0, 0) && led_is(0, 0, 0, 0) && led_is(13, 0, 0, 0), "FX off blanks the row");
    cfg.fx_on = 1; settle();
    cfg.del_mode = DEL_ALWAYS; settle();
    CHECK(led_is(13, cfg.hue[cfg.profile], cfg.sat[cfg.profile], 200), "Delete always mode");
    cfg.del_mode = DEL_FN_ONLY;
    key(PRF_PUB, true); key(PRF_PUB, false); settle();
    CHECK(led_is(13, cfg.hue[0], cfg.sat[0], 200), "Delete flashes after switching");
    now_ms += 2500; settle();
    CHECK(led_is(13, 0x55, 0x55, 0x55), "Delete flash ends after 2s");

    keep_awake  = false;
    layer_state = 1UL << PUB_FN; settle();
    CHECK(!led_is(11, 200, 0, 0), "no warning while keep-awake is off");
    keep_awake = true;
    layer_state = 0; settle();
    for (uint32_t t = 0; t < 4000; t += 40) {
        settle_to(now_ms + 40);
        if (!led_is(11, 0x55, 0x55, 0x55)) { CHECK(false, "F11 touched without FN at t=%u", t); break; }
    }
    layer_state = 1UL << PUB_FN; settle();
    CHECK(led_is(11, 200, 0, 0), "solid red on F11 while FN held");
    cfg.fx_on = 0; settle();
    CHECK(led_is(11, 200, 0, 0), "warning survives FX off");
    cfg.fx_on = 1; keep_awake = false; layer_state = 0; settle();

    hsv_identity = false;
    {
        const uint32_t dts[3] = {16, 32, 50};
        hsv_t prof = {200, 255, 255}, cust = {128, 255, 255};
        for (uint8_t id = ANIM_FIRST_REAL; id <= ANIM_LAST_REAL; id++) {
            uint16_t dur = anim_duration(id);
            CHECK(dur >= 1000 && dur <= 2600, "anim %u duration %u ms", id, dur);
            for (uint8_t mode = 0; mode < ANIM_COLOR_COUNT; mode++) {
                for (int k = 0; k < 3; k++) {
                    rgb_t out[ANIM_LEDS];
                    anim_start(id, mode, prof, cust, 12345 + id * 7 + k);
                    int lit_frames = 0, frames = 0, maxc = 0, last_max = 0;
                    bool early_end = false;
                    for (uint32_t t = 0; t < dur; t += dts[k]) {
                        if (!anim_render(t, out)) { early_end = true; break; }
                        int fm = 0;
                        for (int i = 0; i < ANIM_LEDS; i++) {
                            int m = out[i].r > out[i].g ? out[i].r : out[i].g;
                            m = m > out[i].b ? m : out[i].b;
                            fm = m > fm ? m : fm;
                        }
                        frames++;
                        if (fm > 20) lit_frames++;
                        if (fm > maxc) maxc = fm;
                        if (t + dts[k] >= dur) last_max = fm;
                    }
                    CHECK(!early_end, "anim %u mode %u dt %u ended early", id, mode, dts[k]);
                    CHECK(lit_frames * 100 >= frames * 35, "anim %u mode %u dt %u mostly dark (%d/%d lit)", id, mode, dts[k], lit_frames, frames);
                    CHECK(maxc <= ANIM_LEVEL, "anim %u mode %u exceeds level: %d", id, mode, maxc);
                    CHECK(maxc >= 120, "anim %u mode %u too dim: %d", id, mode, maxc);
                    CHECK(last_max <= 60, "anim %u mode %u dt %u must end dark, last frame %d", id, mode, dts[k], last_max);
                    CHECK(!anim_render(dur, out), "anim %u must report finished at %u ms", id, dur);
                    bool black = true;
                    for (int i = 0; i < ANIM_LEDS; i++) black &= !out[i].r && !out[i].g && !out[i].b;
                    CHECK(black, "anim %u output black after the end", id);
                }
            }
        }
        rgb_t out[ANIM_LEDS];
        anim_start(ANIM_LARSON, ANIM_COLOR_AUTO, prof, cust, 1);
        anim_render(0, out);
        CHECK(out[0].r > 150 && out[0].g < 20 && out[0].b < 20, "Larson auto is red");
        anim_start(ANIM_LARSON, ANIM_COLOR_CUSTOM, prof, cust, 1);
        anim_render(0, out);
        CHECK(out[0].r < 20 && out[0].g > 150 && out[0].b > 150, "Larson custom cyan");
    }

    cfg.startup_anim = ANIM_COMETS; cfg.anim_color = ANIM_COLOR_AUTO; cfg.fx_on = 1; cfg_save();
    rgbm_v = 150;
    boot(false);
    settle();
    CHECK(!anim_active && rgbm_v == 150, "no animation on warm boot / wake");
    now_ms = 500000;
    boot(true);
    CHECK(anim_active && rgbm_v == 0, "power-up: animation running, effect parked at 0");
    frame();
    CHECK(led_is(0, 200, 200, 200) && led_is(13, 200, 200, 200) && led_is(6, 0, 0, 0), "t=0: comets at both ends");
    printf("comets frames (LED brightness 0-200):\n");
    uint32_t t0 = anim_timer;
    for (uint32_t t = 0; t <= 1200; t += 80) {
        now_ms = t0 + t; frame();
        printf("  t=%4u ", t);
        for (int i = 0; i < 14; i++) printf("%3d ", ws2812_leds[i].r == 0x55 ? -1 : ws2812_leds[i].r);
        printf("%s\n", anim_active ? "" : "(done)");
    }
    now_ms = t0 + 520 + 60; frame();
    CHECK(ws2812_leds[6].r > 0 && ws2812_leds[6].r == ws2812_leds[7].r, "comets meet in the middle");
    now_ms = t0 + anim_duration(ANIM_COMETS); frame();
    CHECK(!anim_active && fade_pending, "animation ends, fade queued");
    CHECK(rgbm_v == 0, "still dark until the fade starts");
    housekeeping_task_user();
    CHECK(fade_active, "fade started");
    uint8_t prev = 0; bool mono = true; int steps = 0;
    printf("fade-in (effect brightness):");
    for (uint32_t e = 0; e <= 1000; e += 32) {
        now_ms = fade_timer + e;
        housekeeping_task_user();
        if (rgbm_v < prev) mono = false;
        prev = rgbm_v; steps++;
        if (e % 128 == 0) printf(" %u", rgbm_v);
    }
    printf("\n");
    CHECK(mono, "fade never steps backwards");
    CHECK(rgbm_v == 150 && !fade_active, "fade lands exactly on the user's brightness: %u", rgbm_v);
    now_ms = fade_timer + 90;
    {
        uint32_t p = 90 * 1024 / 900, k = p * p * (3 * 1024 - 2 * p) / (1024 * 1024);
        CHECK(150 * k / 1024 < 150 * 90 / 900, "ease-in is slower than linear at the start");
    }

    cfg.fx_on = 0; cfg_save(); rgbm_v = 150;
    boot(true);
    CHECK(anim_active && rgbm_v == 150 && !fade_armed, "FX off: brightness untouched");
    now_ms = anim_timer + 5000; frame(); housekeeping_task_user();
    CHECK(rgbm_v == 150 && !fade_active, "FX off: no fade");
    cfg.fx_on = 1; cfg_save();

    rgbm_v = 150;
    boot(true);
    CHECK(rgbm_v == 0, "parked");
    key(RGB_VAI, true);
    CHECK(rgbm_v == 150 && !fade_armed, "RGB key restores the real brightness first");
    key(RGB_VAI, false);
    now_ms = anim_timer + 5000; frame(); housekeeping_task_user();
    CHECK(rgbm_v == 150, "no fade after a manual change");

    rgbm_v = 150;
    boot(true);
    now_ms = anim_timer + 5000; frame(); housekeeping_task_user();
    now_ms = fade_timer + 300; housekeeping_task_user();
    rgbm_v = 77;
    now_ms = fade_timer + 400; housekeeping_task_user();
    CHECK(rgbm_v == 77 && !fade_active, "fade does not fight the user");

    rgbm_enabled = false; rgbm_v = 150;
    boot(true);
    CHECK(!anim_active, "no animation with RGB off");
    rgbm_enabled = true;

    cfg.startup_anim = ANIM_SHUFFLE; cfg.shuffle_next = ANIM_LAST_REAL - 1; cfg_save();
    boot(true);
    CHECK(cfg.shuffle_next == ANIM_LAST_REAL && ((rj_config_t *)ee_data)->shuffle_next == ANIM_LAST_REAL, "shuffle advanced + saved");
    boot(true);
    CHECK(cfg.shuffle_next == ANIM_FIRST_REAL, "shuffle wraps to the first animation");
    boot(false);
    CHECK(cfg.shuffle_next == ANIM_FIRST_REAL, "wake does not advance shuffle");

    now_ms += 10000; frame(); housekeeping_task_user(); now_ms += 2000; housekeeping_task_user();
    memset(msg, 0, sizeof msg);
    msg[0] = id_custom_set_value; msg[1] = 0; msg[2] = id_startup_anim; msg[3] = ANIM_FIRE;
    via_custom_value_command_kb(msg, sizeof msg);
    CHECK(cfg.startup_anim == ANIM_FIRE, "VIA set animation");
    msg[3] = ANIM_COUNT; via_custom_value_command_kb(msg, sizeof msg);
    CHECK(cfg.startup_anim == ANIM_FIRE, "VIA rejects unknown animation");
    msg[2] = id_anim_color; msg[3] = ANIM_COLOR_CUSTOM; via_custom_value_command_kb(msg, sizeof msg);
    msg[2] = id_anim_custom; msg[3] = 42; msg[4] = 99; via_custom_value_command_kb(msg, sizeof msg);
    CHECK(cfg.anim_color == ANIM_COLOR_CUSTOM && cfg.anim_hue == 42 && cfg.anim_sat == 99, "VIA anim colour");
    msg[0] = id_custom_get_value; msg[3] = msg[4] = 0; via_custom_value_command_kb(msg, sizeof msg);
    CHECK(msg[3] == 42 && msg[4] == 99, "VIA get anim colour");
    msg[0] = id_custom_set_value; msg[2] = id_anim_play; msg[3] = 1; via_custom_value_command_kb(msg, sizeof msg);
    CHECK(anim_active, "VIA preview button plays the animation");
    now_ms += 5000; frame(); housekeeping_task_user(); now_ms += 2000; housekeeping_task_user();
    cfg.startup_anim = ANIM_OFF;
    CHECK(!key(ANIM_PLAY, true), "ANIM_PLAY consumed");
    key(ANIM_PLAY, false);
    CHECK(anim_active, "play key previews (Comets when set to Off)");
    now_ms += 5000; frame(); housekeeping_task_user(); now_ms += 2000; housekeeping_task_user();

    rj_config_t old = cfg_defaults;
    old.startup_anim = 1;
    memset(&old.anim_color, 0, 4);
    memcpy(ee_data, &old, sizeof old);
    boot(false);
    CHECK(cfg.startup_anim == ANIM_COMETS && cfg.anim_color == ANIM_COLOR_AUTO && cfg.shuffle_next == ANIM_FIRST_REAL,
          "old config migrates cleanly");
    hsv_identity = true;

    hsv_identity = true; caps = false; cur_dev = DEVS_USB; md_state = MD_STATE_CONNECTED;
    cfg.flags = PF_ALL; cfg.fx_on = 1; cfg.fn_dim_pct = 25; rgbm_v = 150; idle_ms = 0;
    layer_state = 1UL << PUB_FN;
    now_ms += 3000; settle();
    CHECK(absd(ws2812_leds[2].r, 0x55 * 25 / 100) <= 1 && led_is(5, 200, 80, 0), "before: FN dim + indicators");
    key(RGB_VAI, true); settle();
    CHECK(led_is(1, 0x55, 0x55, 0x55) && led_is(5, 0x55, 0x55, 0x55) && led_is(9, 0x55, 0x55, 0x55),
          "HUD: F1-F9 show the real effect at full brightness (F1 %u, F5 %u, F9 %u)", ws2812_leds[1].r, ws2812_leds[5].r, ws2812_leds[9].r);
    CHECK(led_is(10, 0, 0, 0) && led_is(12, 0, 0, 0), "HUD: keys past 150/200 are dark");
    CHECK(led_is(0, 0, 0, 0) && led_is(13, 0, 0, 0), "HUD: Esc and Delete off");
    rgbm_steps = 0; rgbm_eeprom_saves = 0;
    for (int k = 0; k < 40; k++) { now_ms += 15; housekeeping_task_user(); }
    CHECK(rgbm_steps >= 5 && rgbm_v == 200, "holding ramps brightness (%d steps, now %u)", rgbm_steps, rgbm_v);
    CHECK(rgbm_eeprom_saves == 0, "no EEPROM writes while ramping");
    key(RGB_VAI, false);
    CHECK(rgbm_eeprom_saves == 1, "one EEPROM write on release");
    settle();
    CHECK(led_is(12, 0x55, 0x55, 0x55), "HUD at full: all twelve keys show the effect");
    settle_to(now_ms + 1500);
    CHECK(absd(ws2812_leds[2].r, 0x55 * 25 / 100) <= 1 && led_is(5, 200, 80, 0), "after the HUD: FN dim and indicators return");
    rgbm_eeprom_saves = 0;
    key(RGB_VAD, true); key(RGB_VAD, false);
    CHECK(rgbm_eeprom_saves == 0, "a single tap leaves the save to QMK");
    settle_to(now_ms + 1500);
    rgbm_speed = 128; key(RGB_SPI, true); settle(); key(RGB_SPI, false);
    CHECK(led_is(1, 0x55, 0x55, 0x55) && led_is(6, 0x55, 0x55, 0x55) && ws2812_leds[7].r < 10 && led_is(12, 0, 0, 0),
          "HUD: speed shows the live effect up to the speed level");
    settle_to(now_ms + 1500);
    key(RGB_HUI, true); settle(); key(RGB_HUI, false);
    CHECK(led_is(4, 0x55, 0x55, 0x55) && led_is(12, 0x55, 0x55, 0x55) && led_is(0, 0, 0, 0),
          "HUD: colour shows the live effect across F1-F12");
    settle_to(now_ms + 1500);
    cfg.fx_on = 0; settle();
    key(RGB_VAI, true); settle(); key(RGB_VAI, false);
    CHECK(led_is(3, 0x55, 0x55, 0x55), "HUD previews the effect even with the effect lights off");
    settle_to(now_ms + 1500);
    CHECK(led_is(3, 0, 0, 0), "effect lights off again after the HUD");
    cfg.fx_on = 1;
    cfg.flags &= ~PF_HUD;
    key(RGB_VAI, true); settle(); key(RGB_VAI, false);
    CHECK(absd(ws2812_leds[2].r, 0x55 * 25 / 100) <= 1, "HUD off: nothing changes");
    cfg.flags = PF_ALL; rgbm_v = 150; layer_state = 0; settle();

    cfg.fx_on = 0; cfg_save();
    boot(false);
    now_ms += 16; frame();
    CHECK(led_is(6, 0, 0, 0), "FX off: dark from the very first frame (got %u)", ws2812_leds[6].r);
    cfg.fx_on = 1; cfg_save(); boot(false); settle();

    settle_to(now_ms + 3000);
    {
        uint8_t  best[14] = {0};
        uint32_t when[14] = {0}, t0s;
        key(PRF_PER, true); key(PRF_PER, false);
        t0s = now_ms;
        for (int f = 0; f < 30; f++) {
            now_ms += 16; frame();
            for (int i = 0; i < 13; i++) if (ws2812_leds[i].g > best[i]) { best[i] = ws2812_leds[i].g; when[i] = now_ms - t0s; }
        }
        bool all = true, ordered = true;
        for (int i = 0; i < 13; i++) { all &= best[i] > 200; if (i && when[i] < when[i - 1]) ordered = false; }
        CHECK(all, "sweep reaches every key (F6 peak %u)", best[6]);
        CHECK(ordered, "sweep runs left to right");
        settle();
        CHECK(led_is(6, 0x55, 0x55, 0x55), "sweep fades back to the effect");
    }
    cfg.flags &= ~PF_SWEEP;
    key(PRF_PUB, true); key(PRF_PUB, false); now_ms += 160; frame();
    CHECK(led_is(5, 0x55, 0x55, 0x55), "sweep off: no sweep");
    cfg.flags = PF_ALL; settle_to(now_ms + 3000);

    cur_dev = DEVS_BT1; md_state = MD_STATE_DISCONNECTED; settle();
    md_state = MD_STATE_CONNECTED; now_ms += 16; frame();
    {
        uint32_t p0 = now_ms;
        uint8_t prev = ws2812_leds[6].b, peak = 0; int turns = 0; bool falling = false; int maxd = 0;
        for (uint32_t t = 16; t <= 1000; t += 16) {
            now_ms = p0 + t; frame();
            uint8_t v = ws2812_leds[6].b;
            if (v > peak) peak = v;
            if (v < prev) falling = true;
            if (falling && v > prev) turns++;
            if (absd(v, prev) > maxd) maxd = absd(v, prev);
            prev = v;
        }
        settle_to(p0 + 1200); uint8_t after = ws2812_leds[6].b;
        CHECK(peak >= 195 && turns == 0, "connect: one swell on F6 (peak %u, bumps after falling %d)", peak, turns);
        CHECK(maxd < 45, "connect swell is gentle (biggest step %d)", maxd);
        CHECK(after == 0x55, "pulse settles back to the effect");
    }
    cur_dev = DEVS_USB; now_ms += 16; frame(); settle_to(now_ms + 300);
    CHECK(ws2812_leds[5].r > 180 && ws2812_leds[5].b < 60, "switching to cable pulses F5 orange");
    settle_to(now_ms + 1200);
    cfg.flags &= ~PF_CONN_PULSE;
    md_state = MD_STATE_CONNECTED; cur_dev = DEVS_2G4; now_ms += 16; frame(); settle_to(now_ms + 90);
    CHECK(led_is(8, 0x55, 0x55, 0x55), "pulse off: no pulse (F8 %u,%u,%u)", ws2812_leds[8].r, ws2812_leds[8].g, ws2812_leds[8].b);
    cfg.flags = PF_ALL; cur_dev = DEVS_USB; settle_to(now_ms + 1500);

    cfg.idle_dim_pct = 30; cfg.idle_dim_s = 30;
    idle_ms = 0; settle();
    CHECK(ws2812_leds[2].r == 0x55, "active: full brightness");
    idle_ms = 30700; settle();
    CHECK(ws2812_leds[2].r < 0x55 && ws2812_leds[2].r > 0x55 * 30 / 100, "dimming eases down (got %u)", ws2812_leds[2].r);
    idle_ms = 40000; settle();
    CHECK(absd(ws2812_leds[2].r, 0x55 * 30 / 100) <= 1, "idle: dimmed to 30%% (got %u)", ws2812_leds[2].r);
    idle_ms = 59000; settle();
    CHECK(ws2812_leds[2].r < 0x55 * 30 / 100 && ws2812_leds[2].r > 0, "goodbye fade in progress (got %u)", ws2812_leds[2].r);
    idle_ms = 60000; settle();
    CHECK(ws2812_leds[2].r == 0, "dark before the RGB timeout cuts at 61 s");
    idle_ms = 0;
    {
        uint8_t prev = 0; bool mono = true;
        for (int f = 0; f < 20; f++) { now_ms += 16; frame(); if (ws2812_leds[2].r < prev) mono = false; prev = ws2812_leds[2].r; if (f == 0) CHECK(prev < 0x55 / 2, "wake fades in, first frame %u", prev); }
        CHECK(mono && prev == 0x55, "wake: smooth climb back to full in ~0.2 s");
    }
    cfg.flags &= ~PF_IDLE_DIM;
    idle_ms = 40000; settle();
    CHECK(ws2812_leds[2].r == 0x55, "idle dim off: stays full");
    idle_ms = 59000; settle();
    CHECK(ws2812_leds[2].r < 0x55, "idle dim off: goodbye fade still runs");
    cfg.flags &= ~PF_SLEEP_FADE;
    settle();
    CHECK(ws2812_leds[2].r == 0x55, "both off: lights stay until the timeout");
    cfg.flags = PF_ALL; idle_ms = 0; settle();

    for (int i = 0; i < 14; i++) ws2812_leds[i] = (ws2812_led_t){180, 90, 30};
    flushes = 0;
    {
        uint32_t t0p = now_ms;
        bridge75_suspend_fade();
        bool mono = true;
        for (int k = 1; k < flushes && k < 32; k++) if (flush_trace[k] > flush_trace[k - 1]) mono = false;
        CHECK(flushes == 20 && mono && led_is(0, 0, 0, 0) && led_is(13, 0, 0, 0), "sleep: 20-step fade to black (%d flushes)", flushes);
        CHECK(now_ms - t0p >= 500 && now_ms - t0p <= 700, "sleep fade takes ~0.6 s (%u ms)", now_ms - t0p);
    }
    bridge75_suspend_fade();
    CHECK(flushes == 20, "suspend loop: fade runs once");
    suspend_wakeup_init_user();
    now_ms += 16; frame();
    CHECK(ws2812_leds[2].r < 0x55 / 2, "PC wake: lights ease back in");
    settle();
    CHECK(ws2812_leds[2].r == 0x55, "PC wake: back to full");
    cfg.flags &= ~PF_SLEEP_FADE; flushes = 0;
    bridge75_suspend_fade();
    CHECK(flushes == 0, "sleep fade off: cut straight away");
    suspend_wakeup_init_user(); cfg.flags = PF_ALL; settle();
    suspend_wakeup_init_user();
    now_ms += 16; frame();
    CHECK(ws2812_leds[2].r == 0x55 && ws2812_leds[9].r == 0x55, "USB restart (no real sleep): lights stay, no dip (%u)", ws2812_leds[2].r);

    {
        rj_config_t old = cfg_defaults;
        old.profile = PROFILE_GAMING; old.hue[0] = 11; old.startup_anim = ANIM_FIRE; old.socd_mode = SOCD_NEUTRAL;
        memset(ee_raw, 0xAA, sizeof ee_raw);
        memcpy(ee_raw, &old, 16);
        ee_valid = false; ee_version = 16; ee_writes = 0;
        boot(false);
        CHECK(cfg.profile == PROFILE_GAMING && cfg.hue[0] == 11 && cfg.startup_anim == ANIM_FIRE && cfg.socd_mode == SOCD_NEUTRAL,
              "old settings kept");
        CHECK(cfg.flags == PF_ALL && cfg.idle_dim_pct == 30 && cfg.idle_dim_s == 30 && cfg.fn_dim_pct == 25, "new settings start at defaults");
        CHECK(ee_valid && ee_version == 32 && ee_writes == 1, "migrated settings saved in the new layout");
        ee_valid = false; ee_version = 7; boot(false);
        CHECK(cfg.profile == PROFILE_PUBLIC && cfg.flags == PF_ALL, "unknown layout -> defaults");
    }

    memset(msg, 0, sizeof msg);
    msg[0] = id_custom_set_value; msg[1] = 0; msg[2] = id_fn_dim_pct; msg[3] = 50;
    via_custom_value_command_kb(msg, sizeof msg);
    CHECK(cfg.fn_dim_pct == 50, "VIA FN dim level");
    msg[3] = 101; via_custom_value_command_kb(msg, sizeof msg);
    CHECK(cfg.fn_dim_pct == 50, "VIA rejects FN dim > 100%%");
    msg[2] = id_idle_dim_s; msg[3] = 55; via_custom_value_command_kb(msg, sizeof msg);
    CHECK(cfg.idle_dim_s == 30, "VIA rejects idle delay past 50 s");
    msg[2] = id_idle_dim_s; msg[3] = 10; via_custom_value_command_kb(msg, sizeof msg);
    CHECK(cfg.idle_dim_s == 10, "VIA idle delay");
    msg[2] = id_hud; msg[3] = 0; via_custom_value_command_kb(msg, sizeof msg);
    CHECK(!(cfg.flags & PF_HUD), "VIA HUD toggle off");
    msg[0] = id_custom_get_value; msg[3] = 9; via_custom_value_command_kb(msg, sizeof msg);
    CHECK(msg[3] == 0, "VIA get HUD toggle");
    msg[0] = id_custom_set_value; msg[2] = id_wake_replay; msg[3] = 0; via_custom_value_command_kb(msg, sizeof msg);
    CHECK(!bridge75_wake_replay, "VIA wake replay off reaches the kb code");
    msg[3] = 1; via_custom_value_command_kb(msg, sizeof msg);
    CHECK(bridge75_wake_replay, "VIA wake replay on");

    cfg.flags = PF_ALL; cfg.fn_dim_pct = 25; idle_ms = 0; layer_state = 0; caps = false; cur_dev = DEVS_USB;
    settle();
    layer_state = 1UL << PUB_FN;
    for (int f = 0; f < 120; f++) {
        now_ms += 16;
        ws2812_leds[0] = (ws2812_led_t){0x55, 0x55, 0x55};
        rgb_matrix_indicators_advanced_user(0, 14);
    }
    CHECK(absd(ws2812_leds[3].r, 0x55 * 25 / 100) <= 1, "FN dim holds at 25%% on an unrepainted key (got %u, would compound to 0)", ws2812_leds[3].r);
    layer_state = 0;
    for (int f = 0; f < 40; f++) { now_ms += 16; ws2812_leds[0] = (ws2812_led_t){0x55, 0x55, 0x55}; rgb_matrix_indicators_advanced_user(0, 14); }
    CHECK(ws2812_leds[3].r == 0x55, "unrepainted key comes back to full after FN (got %u)", ws2812_leds[3].r);
    settle();

    rgbm_v = 150; rgbm_eeprom_saves = 0; mash_active = false;
    key(RGB_VAD, true);
    for (int k = 0; k < 30; k++) { now_ms += 15; housekeeping_task_user(); }
    CHECK(rgbm_v < 150, "ramping down while held (now %u)", rgbm_v);
    mash_active = true;
    now_ms += 15; housekeeping_task_user();
    CHECK(rgbm_v == 150, "mash: brightness restored (got %u)", rgbm_v);
    for (int k = 0; k < 30; k++) { now_ms += 15; housekeeping_task_user(); }
    CHECK(rgbm_v == 150, "mash: no more steps");
    key(RGB_VAD, false);
    CHECK(rgbm_eeprom_saves == 0, "mash: nothing saved");
    mash_active = false; settle_to(now_ms + 2000);

    layer_state = 0; caps = false; cfg.fx_on = 1; idle_ms = 0; settle();
    caps = true;
    now_ms += 16; frame();
    CHECK(ws2812_leds[0].r >= 0x55 + (200 - 0x55) * 14 / 100 && ws2812_leds[0].r < 200,
          "caps: ~40%% (perceived) after one 16 ms frame (got %u)", ws2812_leds[0].r);
    now_ms += 16; frame(); now_ms += 16; frame();
    CHECK(led_is(0, 200, 200, 200), "caps: full within ~48 ms (got %u)", ws2812_leds[0].r);
    caps = false;
    now_ms += 16; frame();
    CHECK(ws2812_leds[0].r > 170, "caps off: softer release, still bright after 16 ms (got %u)", ws2812_leds[0].r);
    settle_to(now_ms + 160);
    CHECK(led_is(0, 0x55, 0x55, 0x55), "caps off: gone within ~160 ms");
    layer_state = 1UL << PUB_FN; now_ms += 16; frame();
    CHECK(ws2812_leds[13].g < 0x55 + (255 - 0x55) * 25 / 100, "Delete keeps the gentler fade-in");
    layer_state = 0; settle();

    hsv_identity = true; caps = false; cur_dev = DEVS_USB; md_state = MD_STATE_CONNECTED; idle_ms = 0;
    cfg.flags = PF_ALL; cfg.fx_on = 1; cfg.fn_dim_pct = 25; cfg.nav = 0; cfg.del_mode = DEL_FN_ONLY;
    layer_state = 0; settle();
    uint8_t dim25 = 0x55 * 25 / 100;
    layer_state = 1UL << PER_FN; settle();
    CHECK(led_is(5, 200, 80, 0) && led_is(13, cfg.hue[cfg.profile], cfg.sat[cfg.profile], 200) && led_is(0, 0, 200, 0) == (md_bat > 90),
          "FN held: indicators visible");
    CHECK(key(KC_LEFT, true), "FN+H still sends Left");
    settle();
    CHECK(led_is(5, 120, 120, 120), "H echoes on F5 (got %u)", ws2812_leds[5].r);
    CHECK(absd(ws2812_leds[8].r, dim25) <= 1 && absd(ws2812_leds[13].r, dim25) <= 1 && absd(ws2812_leds[0].r, dim25) <= 1,
          "FN lights hidden while navigating (F8 %u, Del %u, Esc %u)", ws2812_leds[8].r, ws2812_leds[13].r, ws2812_leds[0].r);
    CHECK(absd(ws2812_leds[2].r, dim25) <= 1, "default: effect stays dimmed like FN");
    key(KC_LEFT, false);
    key(KC_RGHT, true); settle();
    CHECK(led_is(8, 120, 120, 120) && absd(ws2812_leds[5].r, dim25) <= 1, "L echoes on F8, F5 released");
    key(KC_RGHT, false);
    key(KC_DOWN, true); key(KC_UP, true); settle();
    CHECK(led_is(6, 120, 120, 120) && led_is(7, 120, 120, 120), "J -> F6, K -> F7, both held");
    key(KC_DOWN, false); key(KC_UP, false);
    settle_to(now_ms + 400);
    CHECK(absd(ws2812_leds[8].r, dim25) <= 1, "between strokes: focus lingers, FN lights stay hidden");
    settle_to(now_ms + 800);
    CHECK(led_is(5, 200, 80, 0), "after ~0.7 s without arrows: FN lights come back");

    cfg.nav = NAV_FX_OFF << 2;
    key(KC_LEFT, true); settle();
    CHECK(ws2812_leds[2].r == 0 && led_is(5, 120, 120, 120), "effect mode Off: row dark except the echo");
    key(KC_LEFT, false); settle_to(now_ms + 1200);
    cfg.nav = NAV_FX_NORMAL << 2;
    key(KC_LEFT, true); settle();
    CHECK(ws2812_leds[2].r == 0x55, "effect mode Normal: full effect while navigating");
    key(KC_LEFT, false); settle_to(now_ms + 1200);
    cfg.nav = NAV_NO_ECHO;
    key(KC_LEFT, true); settle();
    CHECK(absd(ws2812_leds[5].r, dim25) <= 1 && absd(ws2812_leds[8].r, dim25) <= 1, "echo off: nothing lit, FN lights still hidden");
    key(KC_LEFT, false); settle_to(now_ms + 1200);
    cfg.nav = NAV_OFF;
    key(KC_LEFT, true); settle();
    CHECK(led_is(5, 200, 80, 0), "focus off: FN lights stay (cable slot, not the white echo)");
    key(KC_LEFT, false); settle();
    cfg.nav = 0;

    layer_state = 0; settle();
    key(KC_LEFT, true); settle();
    CHECK(ws2812_leds[5].r == 0x55, "arrow without FN (dedicated arrow keys): no focus, no echo");
    key(KC_LEFT, false);
    layer_state = 1UL << PER_FN; key(KC_LEFT, true); settle();
    layer_state = 0; settle();
    CHECK(ws2812_leds[5].r == 0x55 && ws2812_leds[2].r == 0x55, "FN released: focus ends right away");
    key(KC_LEFT, false); settle();

    memset(msg, 0, sizeof msg);
    msg[0] = id_custom_set_value; msg[1] = 0; msg[2] = id_nav_fx; msg[3] = NAV_FX_OFF;
    via_custom_value_command_kb(msg, sizeof msg);
    CHECK(NAV_FX(cfg.nav) == NAV_FX_OFF && !(cfg.nav & NAV_OFF), "VIA nav effect mode");
    msg[3] = 3; via_custom_value_command_kb(msg, sizeof msg);
    CHECK(NAV_FX(cfg.nav) == NAV_FX_OFF, "VIA rejects unknown nav effect mode");
    msg[2] = id_nav_echo; msg[3] = 0; via_custom_value_command_kb(msg, sizeof msg);
    CHECK(cfg.nav & NAV_NO_ECHO, "VIA echo off");
    msg[0] = id_custom_get_value; msg[3] = 9; via_custom_value_command_kb(msg, sizeof msg);
    CHECK(msg[3] == 0, "VIA get echo");
    msg[0] = id_custom_set_value; msg[2] = id_nav_focus; msg[3] = 0; via_custom_value_command_kb(msg, sizeof msg);
    CHECK(cfg.nav & NAV_OFF, "VIA focus off");
    msg[0] = id_custom_get_value; msg[3] = 9; via_custom_value_command_kb(msg, sizeof msg);
    CHECK(msg[3] == 0, "VIA get focus");
    cfg.nav = 0;
    {
        rj_config_t saved = cfg_defaults;
        CHECK(saved.nav == 0, "defaults: focus on, echo on, dimmed (all-zero byte = old saved settings)");
    }

    hsv_identity = true; caps = false; cur_dev = DEVS_USB; md_state = MD_STATE_CONNECTED; idle_ms = 0;
    cfg.flags = PF_ALL; cfg.fx_on = 1; cfg.fn_dim_pct = 25; cfg.nav = 0;
    layer_state = 0; settle();
    layer_state = 1UL << PER_FN; settle();
    key(FX_TOG, true); key(FX_TOG, false);
    {
        uint8_t prev = ws2812_leds[2].r; bool mono = true; int released_at = -1;
        for (int k = 0; k < 40; k++) {
            if (k == 6) { layer_state = 0; released_at = ws2812_leds[2].r; }
            now_ms += 16; frame();
            if (ws2812_leds[2].r > prev) { mono = false; CHECK(false, "brightened at frame %d (%u -> %u)", k, prev, ws2812_leds[2].r); break; }
            prev = ws2812_leds[2].r;
        }
        CHECK(mono && released_at > 0, "F11 fade-out with FN let go half way never brightens (released at %d)", released_at);
        CHECK(ws2812_leds[2].r == 0, "and ends dark");
    }
    layer_state = 1UL << PER_FN; settle();
    key(FX_TOG, true); key(FX_TOG, false);
    {
        uint8_t prev = ws2812_leds[2].r; bool mono = true;
        for (int k = 0; k < 40; k++) {
            if (k == 4) layer_state = 0;
            now_ms += 16; frame();
            if (ws2812_leds[2].r < prev) { mono = false; break; }
            prev = ws2812_leds[2].r;
        }
        CHECK(mono && ws2812_leds[2].r == 0x55, "F11 fade-in, FN let go half way: one smooth rise to full (%u)", ws2812_leds[2].r);
    }
    layer_state = 0; settle();

    cfg.fx_on = 1; settle();
    cfg.fx_on = 0;
    for (int k = 0; k < 9; k++) { now_ms += 16; frame(); }
    CHECK(ws2812_leds[2].r > 0x55 / 8 && ws2812_leds[2].r < 0x55 * 40 / 100, "FX fade: about a quarter of the light at half time (got %u)", ws2812_leds[2].r);
    settle(); cfg.fx_on = 1; settle();
    CHECK(ws2812_leds[2].r == 0x55, "FX back to full");
    cfg.fn_dim_pct = 25; layer_state = 1UL << PER_FN; settle();
    CHECK(absd(ws2812_leds[2].r, 0x55 * 25 / 100) <= 1, "FN dim end point unchanged by the perceptual curve (%u)", ws2812_leds[2].r);
    layer_state = 0; settle();

    rgbm_mode = RGB_MATRIX_CYCLE_ALL; settle();
    eff_col = 0xC0; rgbm_mode = RGB_MATRIX_CUSTOM_BAR_AURORA;
    now_ms += 16; frame();
    CHECK(ws2812_leds[3].r >= 0x55 && ws2812_leds[3].r < 0x70, "new effect starts from the old frame (got %u)", ws2812_leds[3].r);
    settle_to(now_ms + 180);
    CHECK(ws2812_leds[3].r > 0x70 && ws2812_leds[3].r < 0xC0, "half way through the crossfade (got %u)", ws2812_leds[3].r);
    settle_to(now_ms + 300);
    CHECK(ws2812_leds[3].r == 0xC0, "crossfade done (got %u)", ws2812_leds[3].r);
    eff_col = 0x55; rgbm_mode = RGB_MATRIX_CYCLE_LEFT_RIGHT; settle();

    {
        memset(dkm, 0, sizeof dkm);
        for (int l = PUB_FN; l <= GAM_FN; l++) dkm[l][1][0] = BT_USB;
        dkm[GAM_FN][1][0] = KC_GRV;
        cfg.rev = 0; rgbm_mode = 2; cfg_save();
        rgbm_mode_saves = 0;
        boot(false);
        CHECK(rev1_effect(2) == RGB_MATRIX_CUSTOM_BAR_BREATH, "old Breathing -> Calm Breath");
        CHECK(rgbm_mode == RGB_MATRIX_CUSTOM_BAR_OCEAN && rgbm_speed == JSCIO_SPEED, "then rev 3: Ocean at 75%% speed (%u, %u)", rgbm_mode, rgbm_speed);
        CHECK(dkm[PUB_FN][1][0] == BT_SW && dkm[PER_FN][1][0] == BT_SW, "FN+~ becomes the cable/dongle toggle");
        CHECK(dkm[GAM_FN][1][0] == KC_GRV, "a remapped FN+~ is kept");
        CHECK(cfg.rev == CFG_REV, "migration recorded");
        rgbm_mode = RGB_MATRIX_CYCLE_ALL; rgbm_mode_saves = 0;
        boot(false);
        CHECK(rgbm_mode == RGB_MATRIX_CYCLE_ALL && rgbm_mode_saves == 0, "runs once only");
        static const uint8_t expect[18] = {0, 1, 12, 2, 3, 2, 3, 4, 4, 2, 3, 5, 6, 9, 15, 14, 14, 14};
        bool ok = true;
        for (uint8_t m = 0; m < 18; m++) {
            if (rev1_effect(m) != expect[m]) { ok = false; CHECK(false, "old effect %u -> %u, expected %u", m, rev1_effect(m), expect[m]); }
        }
        CHECK(ok, "every old effect maps to the closest new one");
        rj_config_t d = cfg_defaults;
        CHECK(d.rev == CFG_REV, "fresh settings need no migration");
        rgbm_mode = RGB_MATRIX_CYCLE_LEFT_RIGHT;
    }
    CHECK(((const uint16_t *)keymaps[PER_FN])[14] == BT_SW, "default keymap: FN+~ = cable/dongle toggle");

    hsv_identity = false;
    {
        rgb_t o[BFX_LEDS], prev[BFX_LEDS];
        for (uint8_t id = 0; id < BFX_COUNT; id++) {
            uint32_t t = 100000u * (id + 1);
            int maxd = 0, lit = 0, peak = 0;
            bfx_render(id, t, 170, 255, 128, prev);
            for (int f = 1; f < 600; f++) {
                if (bfx_is_reactive(id) && f % 9 == 0) bfx_key_hit((uint8_t)(20 + (f * 37) % 180));
                bfx_render(id, t + f * 16, 170, 255, 128, o);
                for (int i = 0; i < BFX_LEDS; i++) {
                    int v = o[i].r > o[i].g ? (o[i].r > o[i].b ? o[i].r : o[i].b) : (o[i].g > o[i].b ? o[i].g : o[i].b);
                    int pv = prev[i].r > prev[i].g ? (prev[i].r > prev[i].b ? prev[i].r : prev[i].b) : (prev[i].g > prev[i].b ? prev[i].g : prev[i].b);
                    if (abs(v - pv) > maxd) maxd = abs(v - pv);
                    if (v > 8) lit++;
                    if (v > peak) peak = v;
                }
                memcpy(prev, o, sizeof o);
            }
            int limit = bfx_is_reactive(id) ? 140 : (id == BFX_TWINKLE || id == BFX_EMBER ? 40 : 20);
            CHECK(maxd <= limit, "effect %u: biggest step between frames %d (limit %d)", id, maxd, limit);
            CHECK(lit > 600 * 14 / 3, "effect %u: the bar is lit most of the time (%d)", id, lit);
            CHECK(peak > 120, "effect %u: reaches a real peak (%d)", id, peak);
        }
        bfx_render(BFX_AURORA, 900000, 0, 0, 128, prev);
        bfx_render(BFX_AURORA, 900016, 0, 0, 255, o);
        int d = 0;
        for (int i = 0; i < BFX_LEDS; i++) d += abs(o[i].g - prev[i].g);
        CHECK(d < 14 * 6, "speed change: no jump (%d)", d);
        bfx_render(BFX_LAVA, 1000000, 0, 0, 128, prev);
        bfx_render(BFX_LAVA, 1600000, 0, 0, 128, o);
        d = 0;
        for (int i = 0; i < BFX_LEDS; i++) d += abs(o[i].r - prev[i].r);
        CHECK(d < 14 * 10, "long pause: carries on smoothly (%d)", d);
        bool dark = false;
        for (int f = 0; f < 2000; f++) {
            bfx_render(BFX_EMBER, 2000000 + f * 16, 0, 0, 255, o);
            for (int i = 0; i < BFX_LEDS; i++) if (o[i].r < 20) dark = true;
        }
        CHECK(!dark, "ember: coals always glow");
        for (int f = 0; f < 200; f++) bfx_render(BFX_RIPPLES, 3000000 + f * 16, 0, 0, 128, o);
        bfx_key_hit(182);
        bfx_render(BFX_RIPPLES, 3000000 + 200 * 16, 0, 0, 128, o);
        bfx_render(BFX_RIPPLES, 3000000 + 201 * 16, 0, 0, 128, o);
        bfx_render(BFX_RIPPLES, 3000000 + 202 * 16, 0, 0, 128, o);
        CHECK(o[11].r > 60 && o[0].r < 20, "ripple starts above the key (F11 %u, Esc %u)", o[11].r, o[0].r);
        for (int f = 0; f < 200; f++) bfx_render(BFX_GLOW, 4000000 + f * 16, 0, 255, 128, o);
        for (int k = 0; k < 20; k++) { bfx_key_hit(56); bfx_render(BFX_GLOW, 4003200 + k * 100, 0, 255, 128, o); }
        CHECK(o[3].r > 150 && o[12].r < 30, "glow: warm above the hands (F3 %u), cool elsewhere (F12 %u)", o[3].r, o[12].r);
        for (int f = 0; f < 600; f++) bfx_render(BFX_GLOW, 4006000 + f * 16, 0, 255, 128, o);
        CHECK(o[3].r < 40, "glow cools when typing stops (%u)", o[3].r);
        CHECK(key_bar_x[0][14] == 217 && key_bar_x[0][0] == 7 && key_bar_x[2][1] == 28, "key positions match the LEDs");
    }

    hsv_identity = true;

    {
        bool ok = true;
        const uint8_t ids[3] = {ANIM_IGNITION, ANIM_WELCOME, ANIM_SUNRISE};
        for (int n = 0; n < 3; n++) for (uint8_t m = 0; m < ANIM_COLOR_COUNT; m++) {
            rgb_t o[ANIM_LEDS]; int peak = 0;
            anim_start(ids[n], m, (hsv_t){170, 255, 255}, (hsv_t){128, 255, 255}, 7);
            uint32_t t = 0;
            for (; anim_render(t, o); t += 16) for (int i = 0; i < ANIM_LEDS; i++) if (o[i].r + o[i].g + o[i].b > peak) peak = o[i].r + o[i].g + o[i].b;
            if (peak < 150 || t != (anim_duration(ids[n]) + 15u) / 16 * 16) ok = false;
        }
        CHECK(ok, "Ignition / Welcome / Sunrise run their full length and light up");
    }

    {
        const uint16_t *fn = (const uint16_t *)keymaps[PER_FN];
        CHECK(fn[59] == RGB_RMOD && fn[60] == RGB_MOD, "FN+Z / X: previous / next effect");
        CHECK(fn[61] == RGB_HUD && fn[62] == RGB_HUI, "FN+C / V: colour back / forward");
        CHECK(fn[63] == RGB_SPD && fn[64] == RGB_SPI, "FN+B / N: slower / faster");
        CHECK(fn[65] == KC_TRNS && fn[66] == KC_TRNS && fn[67] == KC_TRNS, "FN+M , . free (brightness lives on F9 / F10)");
        CHECK(((const uint16_t *)keymaps[PUB_FN])[59] == RGB_RMOD && ((const uint16_t *)keymaps[GAM_FN])[64] == RGB_SPI, "same on every profile");
    }

    {
        static const uint16_t old_row[9] = {RGB_TOG, RGB_MOD, RGB_RMOD, RGB_HUI, RGB_HUD, RGB_SAI, RGB_SAD, RGB_VAI, RGB_VAD};
        memset(dkm, 0, sizeof dkm);
        for (int l = PUB_FN; l <= GAM_FN; l++) {
            dkm[l][1][0] = BT_SW;
            for (int c = 0; c < 9; c++) dkm[l][4][c + 1] = old_row[c];
        }
        dkm[GAM_FN][4][9] = KC_MUTE;
        dkm[PUB_FN][1][0] = BT_USB;
        rgbm_mode = RGB_MATRIX_CUSTOM_BAR_AURORA; rgbm_mode_saves = 0;
        cfg.rev = 1; cfg_save(); boot(false);
        CHECK(dkm[PUB_FN][4][1] == RGB_RMOD && dkm[PUB_FN][4][2] == RGB_MOD && dkm[PUB_FN][4][5] == RGB_SPD &&
              dkm[PUB_FN][4][6] == RGB_SPI && dkm[PUB_FN][4][7] == KC_TRNS && dkm[PER_FN][4][4] == RGB_HUI,
              "stock Z row -> previous/next pairs");
        CHECK(dkm[GAM_FN][4][1] == RGB_TOG && dkm[GAM_FN][4][9] == KC_MUTE, "customised Z row left alone");
        CHECK(dkm[PUB_FN][1][0] == BT_USB, "rev 1 steps don't run again");
        CHECK(cfg.rev == CFG_REV, "now at the current rev");
        rgbm_mode = RGB_MATRIX_CYCLE_LEFT_RIGHT;
    }

    hsv_identity = true; caps = false; cur_dev = DEVS_USB; md_state = MD_STATE_CONNECTED; idle_ms = 0;
    cfg.flags = PF_ALL; cfg.fx_on = 1; cfg.del_mode = DEL_FN_ONLY; layer_state = 0;
    settle_to(now_ms + 3000);
    {
        uint32_t t0 = now_ms;
        key(PRF_NXT, true); key(PRF_NXT, false);
        uint8_t prof = cfg.profile;
        settle_to(t0 + 250);
        uint8_t before = ws2812_leds[13].b;
        settle_to(t0 + 335 + 90);  uint8_t dip1 = ws2812_leds[13].b;
        settle_to(t0 + 335 + 150); uint8_t up   = ws2812_leds[13].b;
        settle_to(t0 + 335 + 210); uint8_t dip2 = ws2812_leds[13].b;
        settle_to(t0 + 900);       uint8_t hold = ws2812_leds[13].b;
        CHECK(before == 200 && hold == 200 && led_is(13, cfg.hue[prof], cfg.sat[prof], 200), "Delete holds the profile colour around the flicker (before %u, hold %u, now %u,%u,%u)", before, hold, ws2812_leds[13].r, ws2812_leds[13].g, ws2812_leds[13].b);
        CHECK(dip1 < 60 && up > 150 && dip2 > dip1 && dip2 < 140, "two dips, the second shallower (%u, %u, %u)", dip1, up, dip2);
        key(PRF_NXT, true); key(PRF_NXT, false);
        uint8_t prev = ws2812_leds[13].b; int maxd = 0;
        for (int k = 0; k < 50; k++) { now_ms += 16; frame(); int d = absd(ws2812_leds[13].b, prev); if (d > maxd) maxd = d; prev = ws2812_leds[13].b; }
        CHECK(maxd < 110, "flicker is shaped, not a hard blink (biggest step %d)", maxd);
        settle_to(now_ms + 3000);
        layer_state = 1UL << PER_FN; settle();
        t0 = now_ms;
        key(PRF_NXT, true); key(PRF_NXT, false);
        settle_to(t0 + 335 + 90);
        CHECK(ws2812_leds[13].b < 60, "flicker shows with FN held too (%u)", ws2812_leds[13].b);
        layer_state = 0; settle_to(now_ms + 3000);
        cfg.flags &= ~PF_SWEEP;
        t0 = now_ms;
        key(PRF_NXT, true); key(PRF_NXT, false);
        settle_to(t0 + 140 + 90);
        CHECK(ws2812_leds[13].b < 60, "no sweep: flicker right after Delete lights (%u)", ws2812_leds[13].b);
        cfg.flags = PF_ALL; settle_to(now_ms + 3000);
        while (cfg.profile != PROFILE_PUBLIC) { key(PRF_NXT, true); key(PRF_NXT, false); }
        settle_to(now_ms + 3000);
    }

    {
        rj_config_t d = cfg_defaults;
        CHECK(d.startup_anim == ANIM_WELCOME && d.anim_color == ANIM_COLOR_CUSTOM && d.anim_hue == 128 && d.anim_sat == 255,
              "defaults: Welcome in the custom colour, hue 128 / sat 255");
        hsv_identity = false;
        rgb_t c = hsv_to_rgb((hsv_t){128, 255, 255});
        CHECK(c.r == 0x00 && c.g == 0xFC && c.b == 0xFF, "hue 128 / sat 255 is exactly #00FCFF (%02X%02X%02X)", c.r, c.g, c.b);
        anim_start(ANIM_WELCOME, ANIM_COLOR_CUSTOM, (hsv_t){170, 255, 255}, (hsv_t){128, 255, 255}, 1);
        rgb_t o[ANIM_LEDS];
        anim_render(1150, o);
        CHECK(o[6].r == 0 && o[6].b > 150 && abs(o[6].g - o[6].b) <= 4, "Welcome renders cyan (%u,%u,%u)", o[6].r, o[6].g, o[6].b);
        hsv_identity = true;
        cfg.rev = 2; cfg.startup_anim = ANIM_COMETS; cfg.anim_color = ANIM_COLOR_AUTO; cfg_save();
        rgbm_mode = RGB_MATRIX_CYCLE_ALL; rgbm_speed = 128;
        boot(false);
        CHECK(rgbm_mode == RGB_MATRIX_CUSTOM_BAR_OCEAN && rgbm_speed == 191 && cfg.startup_anim == ANIM_WELCOME &&
              cfg.anim_color == ANIM_COLOR_CUSTOM && cfg.anim_hue == 128 && cfg.rev == 3, "rev 3 applies the defaults once");
        rgbm_mode = RGB_MATRIX_CYCLE_ALL; cfg.startup_anim = ANIM_SUNRISE; cfg_save();
        boot(false);
        CHECK(rgbm_mode == RGB_MATRIX_CYCLE_ALL && cfg.startup_anim == ANIM_SUNRISE, "and never again");
        rgbm_mode = RGB_MATRIX_CYCLE_LEFT_RIGHT; rgbm_speed = 128; cfg.startup_anim = ANIM_COMETS; cfg.anim_color = ANIM_COLOR_AUTO; cfg_save();
    }

    printf("%d/%d checks passed\n", checks - failures, checks);
    return failures ? 1 : 0;
}
