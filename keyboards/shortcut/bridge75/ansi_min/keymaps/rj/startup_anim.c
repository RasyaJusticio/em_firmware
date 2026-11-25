// SPDX-License-Identifier: GPL-2.0-or-later

#include "startup_anim.h"
#include <string.h>

#define N ANIM_LEDS
#define CENTER_X100 650

static uint8_t  cur_id;
static uint8_t  cmode;
static hsv_t    col_profile, col_custom;
static uint32_t rng;
static uint32_t last_t;
static uint16_t accum;

static uint8_t lvl[N];
static uint8_t hue[N];
static uint8_t sat[N];

static uint8_t heat[N / 2];

typedef struct {
    int32_t h;
    int32_t v;
    bool    rest;
} ball_t;
static ball_t balls[2];

static uint8_t drop_pos[3], drop_hue[3];

static uint8_t rand8(void) {
    rng ^= rng << 13;
    rng ^= rng >> 17;
    rng ^= rng << 5;
    return rng >> 24;
}

static uint8_t qadd8(uint8_t a, uint8_t b) { return (a + b > 255) ? 255 : a + b; }
static uint8_t qsub8(uint8_t a, uint8_t b) { return (a > b) ? a - b : 0; }
static uint8_t scale8(uint8_t a, uint8_t b) { return ((uint16_t)a * (b + 1)) >> 8; }
static uint8_t lv(uint8_t x) { return ((uint16_t)x * ANIM_LEVEL + 127) / 255; }
static uint8_t max8(uint8_t a, uint8_t b) { return a > b ? a : b; }

static uint8_t ramp(int32_t t, int32_t start, int32_t len) {
    if (t <= start) return 0;
    if (t >= start + len) return 255;
    return (uint8_t)((t - start) * 255 / len);
}

static uint8_t envelope(int32_t t, int32_t dur, int32_t in_ms, int32_t out_ms) {
    uint8_t a = in_ms ? ramp(t, 0, in_ms) : 255;
    uint8_t b = 255 - ramp(t, dur - out_ms, out_ms);
    return a < b ? a : b;
}

static int32_t abs32(int32_t x) { return x < 0 ? -x : x; }

static uint8_t dot(uint8_t i, int32_t x100) {
    int32_t d = abs32(i * 100 - x100);
    return d >= 100 ? 0 : (uint8_t)((100 - d) * 255 / 100);
}

typedef struct {
    uint8_t h, s;
    bool    rainbow;
} palette_t;

static void pick(uint8_t i, uint32_t t, palette_t autop, uint8_t *h, uint8_t *s) {
    switch (cmode) {
        case ANIM_COLOR_PROFILE: *h = col_profile.h; *s = col_profile.s; return;
        case ANIM_COLOR_CUSTOM:  *h = col_custom.h;  *s = col_custom.s;  return;
        case ANIM_COLOR_RAINBOW: autop.rainbow = true; break;
        default: break;
    }
    if (autop.rainbow) {
        *h = (uint8_t)(i * 18 + t / 6);
        *s = 255;
    } else {
        *h = autop.h;
        *s = autop.s;
    }
}

static void put(rgb_t out[N], uint8_t i, uint8_t h, uint8_t s, uint8_t v) {
    out[i] = hsv_to_rgb((hsv_t){h, s, lv(v)});
}

static void put_pal(rgb_t out[N], uint8_t i, uint32_t t, palette_t p, uint8_t v) {
    uint8_t h, s;
    pick(i, t, p, &h, &s);
    put(out, i, h, s, v);
}

static void decay(uint32_t dt, uint16_t ms_full) {
    uint32_t d = dt * 255 / ms_full;
    if (d > 255) d = 255;
    for (uint8_t i = 0; i < N; i++) lvl[i] = qsub8(lvl[i], (uint8_t)d);
}

static const palette_t P_WHITE   = {0, 0, false};
static const palette_t P_RAINBOW = {0, 255, true};
static const palette_t P_RED     = {0, 255, false};
static const palette_t P_ICE     = {140, 110, false};

#define COMET_RUN 520
#define COMET_HOLD 120
static void anim_comets(uint32_t t, rgb_t out[N]) {
    int32_t head;
    if (t < COMET_RUN) head = (int32_t)CENTER_X100 * t / COMET_RUN;
    else if (t < COMET_RUN + COMET_HOLD) head = CENTER_X100;
    else head = CENTER_X100 - (int32_t)CENTER_X100 * (t - COMET_RUN - COMET_HOLD) / COMET_RUN;
    if (head < 0) head = 0;
    int32_t mirror = (N - 1) * 100 - head;
    uint8_t env    = envelope(t, 2 * COMET_RUN + COMET_HOLD, 0, 220);
    for (uint8_t i = 0; i < N; i++) {
        int32_t d  = abs32(i * 100 - head);
        int32_t d2 = abs32(i * 100 - mirror);
        if (d2 < d) d = d2;
        uint8_t v = d >= 200 ? 0 : (uint8_t)((200 - d) * 255 / 200);
        put_pal(out, i, t, P_WHITE, scale8(v, env));
    }
}

static void anim_rainbow_wipe(uint32_t t, rgb_t out[N]) {
    for (uint8_t i = 0; i < N; i++) {
        uint8_t in  = ramp(t, i * 45, 120);
        uint8_t off = 255 - ramp(t, 1000 + i * 45, 120);
        put_pal(out, i, t, P_RAINBOW, in < off ? in : off);
    }
}

static void anim_larson(uint32_t t, uint32_t dt, rgb_t out[N]) {
    decay(dt, 260);
    uint32_t p   = t % 1000;
    int32_t  pos = p < 500 ? (int32_t)p * 1300 / 500 : (int32_t)(1000 - p) * 1300 / 500;
    for (uint8_t i = 0; i < N; i++) lvl[i] = max8(lvl[i], dot(i, pos));
    uint8_t env = envelope(t, 2000, 0, 250);
    for (uint8_t i = 0; i < N; i++) put_pal(out, i, t, P_RED, scale8(lvl[i], env));
}

static void anim_meteor(uint32_t t, uint32_t dt, rgb_t out[N]) {
    for (uint8_t i = 0; i < N; i++) {
        uint16_t d = (uint16_t)((dt * (64u + rand8())) >> 8);
        lvl[i]     = qsub8(lvl[i], d > 255 ? 255 : (uint8_t)d);
    }
    if (t < 1100) {
        int32_t pos = (int32_t)t * 1900 / 1100;
        for (uint8_t i = 0; i < N; i++) lvl[i] = max8(lvl[i], dot(i, pos));
    }
    uint8_t env = envelope(t, 1500, 0, 200);
    for (uint8_t i = 0; i < N; i++) put_pal(out, i, t, P_ICE, scale8(lvl[i], env));
}

static void anim_curtain(uint32_t t, rgb_t out[N]) {
    palette_t p = {col_profile.h, col_profile.s, false};
    for (uint8_t i = 0; i < N; i++) {
        uint8_t ring = (uint8_t)(abs32(i * 2 - (N - 1)) / 2);
        uint8_t in   = ramp(t, ring * 70, 110);
        uint8_t off  = 255 - ramp(t, 950 + ring * 70, 130);
        put_pal(out, i, t, p, in < off ? in : off);
    }
}

static void heat_color(uint8_t h, rgb_t *c) {
    uint8_t t192 = scale8(h, 191);
    uint8_t r    = (t192 & 0x3F) << 2;
    if (t192 & 0x80) {
        *c = (rgb_t){255, 255, r};
    } else if (t192 & 0x40) {
        *c = (rgb_t){255, r, 0};
    } else {
        *c = (rgb_t){r, 0, 0};
    }
}

static void anim_fire(uint32_t t, uint32_t dt, rgb_t out[N]) {
    accum += dt;
    while (accum >= 30) {
        accum -= 30;
        const uint8_t half = N / 2;
        uint8_t       cool = t < 1300 ? 55 : 120;
        for (uint8_t k = 0; k < half; k++) heat[k] = qsub8(heat[k], rand8() % ((cool * 10 / half) + 2));
        for (uint8_t k = half - 1; k >= 2; k--) heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
        if (t < 1300 && rand8() < 170) {
            uint8_t y = rand8() % 2;
            heat[y]   = qadd8(heat[y], 160 + rand8() % 95);
        }
    }
    uint8_t env = envelope(t, 2000, 0, 300);
    for (uint8_t i = 0; i < N; i++) {
        uint8_t k = i < N / 2 ? (N / 2 - 1 - i) : (i - N / 2);
        uint8_t h = scale8(heat[k], env);
        if (cmode == ANIM_COLOR_AUTO) {
            rgb_t c;
            heat_color(h, &c);
            out[i] = (rgb_t){lv(c.r), lv(c.g), lv(c.b)};
        } else {
            put_pal(out, i, t, P_RED, h);
        }
    }
}

static void anim_power_up(uint32_t t, rgb_t out[N]) {
    for (uint8_t i = 0; i < N; i++) {
        palette_t p = {(uint8_t)(i * 85 / (N - 1)), 255, false};
        if (t < 1000) {
            put_pal(out, i, t, p, ramp(t, i * 60, 80));
        } else if (t < 1150) {
            put(out, i, 0, 0, 255);
        } else {
            put(out, i, 0, 0, 255 - ramp(t, 1150, 650));
        }
    }
}

static const uint16_t drop_start[3] = {0, 600, 1150};
static void anim_ripple(uint32_t t, rgb_t out[N]) {
    for (uint8_t i = 0; i < N; i++) {
        uint8_t best = 0, bh = 0;
        for (uint8_t k = 0; k < 3; k++) {
            if (t < drop_start[k]) continue;
            uint32_t age = t - drop_start[k];
            if (age >= 900) continue;
            int32_t r   = (int32_t)age * 700 / 900;
            int32_t d   = abs32(i * 100 - drop_pos[k] * 100);
            int32_t off = abs32(d - r);
            uint8_t amp = (uint8_t)(255 - age * 255 / 900);
            uint8_t v   = off >= 100 ? 0 : scale8(amp, (uint8_t)((100 - off) * 255 / 100));
            if (age < 80 && d < 100) v = 255;
            if (v > best) {
                best = v;
                bh   = drop_hue[k];
            }
        }
        if (cmode == ANIM_COLOR_AUTO) {
            put(out, i, bh, 255, best);
        } else {
            put_pal(out, i, t, P_RAINBOW, best);
        }
    }
}

static uint8_t ease(int32_t t, int32_t start, int32_t len) {
    uint32_t f = ramp(t, start, len);
    uint32_t s = (f * f * (768 - 2 * f)) >> 16;
    return s > 255 ? 255 : (uint8_t)s;
}
static uint8_t sq(uint8_t x) { return scale8(x, x); }

#define IGN_MEET 600
#define IGN_BLOOM 450
#define IGN_HOLD 200
#define IGN_FADE 550
static void anim_ignition(uint32_t t, uint32_t dt, rgb_t out[N]) {
    decay(dt, 200);
    if (t < IGN_MEET) {
        uint32_t u   = t * 255 / IGN_MEET;
        int32_t  pos = (int32_t)(u * u / 255) * CENTER_X100 / 255;
        for (uint8_t i = 0; i < N; i++) lvl[i] = max8(lvl[i], max8(dot(i, pos), dot(i, (N - 1) * 100 - pos)));
    }
    palette_t spark = P_ICE;
    palette_t bloom = {col_profile.h, col_profile.s, false};
    for (uint8_t i = 0; i < N; i++) {
        uint8_t v = lvl[i];
        uint8_t b = 0;
        if (t >= IGN_MEET) {
            uint8_t  u = ease(t, IGN_MEET, IGN_BLOOM);
            int32_t  r = (int32_t)u * 850 / 255;
            int32_t  d = abs32(i * 100 - CENTER_X100);
            b          = d >= r ? 0 : (r - d >= 150 ? 255 : (uint8_t)((r - d) * 255 / 150));
            b          = scale8(b, 255 - ease(t, IGN_MEET + IGN_BLOOM + IGN_HOLD, IGN_FADE));
            b          = sq(b);
        }
        if (b >= v) put_pal(out, i, t, bloom, b);
        else put_pal(out, i, t, spark, v);
    }
}

#define WEL_FILL 800
#define WEL_LIFT 300
#define WEL_HOLD 200
#define WEL_OFF_STEP 25
#define WEL_OFF 400
#define WEL_OFF_AT (WEL_FILL + WEL_LIFT + WEL_HOLD)
static const palette_t P_WARM = {24, 70, false};
static void anim_welcome(uint32_t t, rgb_t out[N]) {
    uint8_t lift = 180 + scale8(ease(t, WEL_FILL, WEL_LIFT), 75);
    int32_t edge = (int32_t)ease(t, 0, WEL_FILL) * 1500 / 255;
    for (uint8_t i = 0; i < N; i++) {
        int32_t in  = edge - i * 100;
        uint8_t f   = in <= 0 ? 0 : (in >= 200 ? 255 : (uint8_t)(in * 255 / 200));
        f           = ease(f, 0, 255);
        uint8_t off = 255 - ease(t, WEL_OFF_AT + i * WEL_OFF_STEP, WEL_OFF);
        put_pal(out, i, t, P_WARM, sq(scale8(scale8(f, lift), off)));
    }
}

static const uint8_t SUN[][3] = {{90, 0, 0}, {200, 30, 0}, {255, 100, 10}, {255, 170, 60}, {255, 220, 160}};
#define SUN_RISE 1500
#define SUN_FADE 700
static void anim_sunrise(uint32_t t, rgb_t out[N]) {
    uint8_t fade = 255 - sq(ease(t, SUN_RISE, SUN_FADE));
    for (uint8_t i = 0; i < N; i++) {
        int32_t d = abs32(i * 100 - CENTER_X100);
        int32_t u = (int32_t)ramp(t, 0, SUN_RISE) * 5 / 4 - d * 64 / 650;
        uint8_t p = u < 0 ? 0 : (u > 255 ? 255 : (uint8_t)u);
        uint8_t v = scale8(sq(ease(p, 0, 255)), fade);
        if (cmode == ANIM_COLOR_AUTO) {
            uint32_t pos = (uint32_t)p * 4 * 256 / 255;
            uint8_t  k = pos >> 8, f = pos & 255;
            if (k >= 4) { k = 3; f = 255; }
            uint8_t r  = SUN[k][0] + (((int32_t)SUN[k + 1][0] - SUN[k][0]) * f >> 8);
            uint8_t g  = SUN[k][1] + (((int32_t)SUN[k + 1][1] - SUN[k][1]) * f >> 8);
            uint8_t bl = SUN[k][2] + (((int32_t)SUN[k + 1][2] - SUN[k][2]) * f >> 8);
            out[i]     = (rgb_t){lv(scale8(r, v)), lv(scale8(g, v)), lv(scale8(bl, v))};
        } else {
            put_pal(out, i, t, P_RED, v);
        }
    }
}

#define BALL_G 3
static void anim_bounce(uint32_t t, uint32_t dt, rgb_t out[N]) {
    decay(dt, 150);
    for (uint8_t b = 0; b < 2; b++) {
        ball_t *ball = &balls[b];
        for (uint32_t ms = 0; ms < dt && !ball->rest; ms++) {
            ball->v -= BALL_G;
            ball->h += ball->v;
            if (ball->h <= 0) {
                ball->h = 0;
                ball->v = -ball->v * (b ? 62 : 70) / 100;
                if (ball->v < 40) ball->rest = true;
            }
        }
        int32_t from_floor = (int32_t)(((int64_t)ball->h * 100) >> 16);
        int32_t pos        = b ? (N - 1) * 100 - from_floor : from_floor;
        for (uint8_t i = 0; i < N; i++) {
            uint8_t v = dot(i, pos);
            if (v > lvl[i]) {
                lvl[i] = v;
                hue[i] = b ? 128 : 10;
            }
        }
    }
    uint8_t env = envelope(t, 2400, 0, 400);
    for (uint8_t i = 0; i < N; i++) {
        if (cmode == ANIM_COLOR_AUTO) {
            put(out, i, hue[i], 255, scale8(lvl[i], env));
        } else {
            put_pal(out, i, t, P_RED, scale8(lvl[i], env));
        }
    }
}

static uint8_t pulse(int32_t t, int32_t at) {
    if (t < at) return 0;
    if (t < at + 60) return ramp(t, at, 60);
    return 255 - ramp(t, at + 60, 260);
}
static void anim_heartbeat(uint32_t t, rgb_t out[N]) {
    uint8_t b = max8(max8(pulse(t, 50), scale8(pulse(t, 330), 180)), max8(pulse(t, 850), scale8(pulse(t, 1130), 180)));
    for (uint8_t i = 0; i < N; i++) {
        int32_t d    = abs32(i * 100 - CENTER_X100);
        uint8_t fall = (uint8_t)(255 - d * 160 / CENTER_X100);
        put_pal(out, i, t, P_RED, scale8(b, fall));
    }
}

uint16_t anim_duration(uint8_t id) {
    switch (id) {
        case ANIM_COMETS:       return 2 * COMET_RUN + COMET_HOLD;
        case ANIM_RAINBOW_WIPE: return 1000 + (N - 1) * 45 + 120;
        case ANIM_LARSON:       return 2000;
        case ANIM_METEOR:       return 1500;
        case ANIM_CURTAIN:      return 950 + 6 * 70 + 130 + 80;
        case ANIM_IGNITION:     return IGN_MEET + IGN_BLOOM + IGN_HOLD + IGN_FADE;
        case ANIM_FIRE:         return 2000;
        case ANIM_POWER_UP:     return 1800;
        case ANIM_WELCOME:      return WEL_OFF_AT + (N - 1) * WEL_OFF_STEP + WEL_OFF;
        case ANIM_RIPPLE:       return 1150 + 900;
        case ANIM_SUNRISE:      return SUN_RISE + SUN_FADE;
        case ANIM_BOUNCE:       return 2400;
        case ANIM_HEARTBEAT:    return 1130 + 60 + 260;
        default:                return 0;
    }
}

void anim_start(uint8_t id, uint8_t color_mode, hsv_t profile, hsv_t custom, uint32_t seed) {
    cur_id      = id;
    cmode       = color_mode < ANIM_COLOR_COUNT ? color_mode : ANIM_COLOR_AUTO;
    col_profile = profile;
    col_custom  = custom;
    rng         = seed ? seed : 0x9E3779B9u;
    last_t      = 0;
    accum       = 0;
    memset(lvl, 0, sizeof lvl);
    memset(hue, 0, sizeof hue);
    memset(sat, 255, sizeof sat);
    memset(heat, 0, sizeof heat);
    for (uint8_t b = 0; b < 2; b++) {
        balls[b] = (ball_t){.h = (int32_t)CENTER_X100 * 65536 / 100, .v = 0, .rest = false};
    }
    for (uint8_t k = 0; k < 3; k++) {
        uint8_t p;
        do {
            p = 1 + rand8() % (N - 2);
        } while (k && (p == drop_pos[k - 1]));
        drop_pos[k] = p;
        drop_hue[k] = rand8();
    }
}

bool anim_render(uint32_t t, rgb_t out[N]) {
    memset(out, 0, sizeof(rgb_t) * N);
    if (cur_id == ANIM_OFF || t >= anim_duration(cur_id)) {
        cur_id = ANIM_OFF;
        return false;
    }
    uint32_t dt = t >= last_t ? t - last_t : 0;
    last_t      = t;

    switch (cur_id) {
        case ANIM_COMETS:       anim_comets(t, out); break;
        case ANIM_RAINBOW_WIPE: anim_rainbow_wipe(t, out); break;
        case ANIM_LARSON:       anim_larson(t, dt, out); break;
        case ANIM_METEOR:       anim_meteor(t, dt, out); break;
        case ANIM_CURTAIN:      anim_curtain(t, out); break;
        case ANIM_IGNITION:     anim_ignition(t, dt, out); break;
        case ANIM_FIRE:         anim_fire(t, dt, out); break;
        case ANIM_POWER_UP:     anim_power_up(t, out); break;
        case ANIM_WELCOME:      anim_welcome(t, out); break;
        case ANIM_RIPPLE:       anim_ripple(t, out); break;
        case ANIM_SUNRISE:      anim_sunrise(t, out); break;
        case ANIM_BOUNCE:       anim_bounce(t, dt, out); break;
        case ANIM_HEARTBEAT:    anim_heartbeat(t, out); break;
        default:                return false;
    }
    return true;
}
