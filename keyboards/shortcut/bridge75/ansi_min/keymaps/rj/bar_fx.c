// SPDX-License-Identifier: GPL-2.0-or-later

#include "bar_fx.h"
#include <string.h>

#define N BFX_LEDS
#define BAR_W 224
#define BAR_MID 112

static const uint8_t led_x[N] = {7, 28, 42, 56, 70, 91, 105, 119, 133, 154, 168, 182, 196, 217};

static uint8_t qadd8(uint8_t a, uint8_t b) { return (a + b > 255) ? 255 : a + b; }
static uint8_t clamp8(int32_t v) { return v < 0 ? 0 : (v > 255 ? 255 : (uint8_t)v); }
static uint8_t scale8(uint8_t a, uint8_t b) { return ((uint16_t)a * (b + 1)) >> 8; }
static uint8_t sq8(uint8_t x) { return scale8(x, x); }
static int32_t abs32(int32_t x) { return x < 0 ? -x : x; }

static int16_t isin(uint16_t ph) {
    uint32_t q = ph & 0x3FFF;
    if (ph & 0x4000) q = 0x4000 - q;
    uint32_t q2 = (q * q) >> 14;
    int32_t  y  = (int32_t)((q * (3 * 16384 - q2)) >> 15);
    int32_t  r  = y * 2 - (y >> 13);
    return (int16_t)((ph & 0x8000) ? -r : r);
}
static uint8_t wave8(uint32_t ph) { return (uint8_t)((isin((uint16_t)ph) + 32768) >> 8); }

static uint8_t smooth8(uint8_t f) {
    uint32_t s = ((uint32_t)f * f * (768 - 2 * f)) >> 16;
    return s > 255 ? 255 : (uint8_t)s;
}
static uint16_t ease16(uint32_t p) {
    if (p >= 65535) return 65535;
    return (uint16_t)((uint64_t)p * p * (3 * 65535ull - 2 * p) / (65535ull * 65535ull));
}

static uint32_t hash32(uint32_t x) {
    x ^= x >> 16;
    x *= 0x7FEB352Du;
    x ^= x >> 15;
    x *= 0x846CA68Bu;
    x ^= x >> 16;
    return x;
}
static uint8_t lerp8(uint8_t a, uint8_t b, uint8_t f) { return (uint8_t)(a + (((int32_t)b - a) * f) / 255); }

static uint8_t noise2(uint32_t x, uint32_t y) {
    uint32_t xi = x >> 8, yi = y >> 8;
    uint8_t  xf = smooth8(x & 255), yf = smooth8(y & 255);
    uint8_t  a  = hash32(xi * 0x9E3779B1u ^ yi * 0x85EBCA77u) >> 24;
    uint8_t  b  = hash32((xi + 1) * 0x9E3779B1u ^ yi * 0x85EBCA77u) >> 24;
    uint8_t  c  = hash32(xi * 0x9E3779B1u ^ (yi + 1) * 0x85EBCA77u) >> 24;
    uint8_t  d  = hash32((xi + 1) * 0x9E3779B1u ^ (yi + 1) * 0x85EBCA77u) >> 24;
    return lerp8(lerp8(a, b, xf), lerp8(c, d, xf), yf);
}

static uint32_t rng = 0x2545F491u;
static uint8_t  rand8(void) {
    rng ^= rng << 13;
    rng ^= rng >> 17;
    rng ^= rng << 5;
    return rng >> 24;
}

typedef uint8_t stop_t[3];

static rgb_t pal(const stop_t *p, uint8_t n, uint8_t idx) {
    uint32_t pos = (uint32_t)idx * (n - 1) * 256 / 255;
    uint8_t  k   = pos >> 8;
    uint8_t  f   = pos & 255;
    if (k >= n - 1) return (rgb_t){.r = p[n - 1][0], .g = p[n - 1][1], .b = p[n - 1][2]};
    rgb_t c;
    c.r = (uint8_t)(p[k][0] + (((int32_t)p[k + 1][0] - p[k][0]) * f >> 8));
    c.g = (uint8_t)(p[k][1] + (((int32_t)p[k + 1][1] - p[k][1]) * f >> 8));
    c.b = (uint8_t)(p[k][2] + (((int32_t)p[k + 1][2] - p[k][2]) * f >> 8));
    return c;
}

static rgb_t scale_c(rgb_t c, uint8_t v) {
    c.r = scale8(c.r, v);
    c.g = scale8(c.g, v);
    c.b = scale8(c.b, v);
    return c;
}
static rgb_t mix_c(rgb_t a, rgb_t b, uint8_t f) {
    a.r = lerp8(a.r, b.r, f);
    a.g = lerp8(a.g, b.g, f);
    a.b = lerp8(a.b, b.b, f);
    return a;
}
static rgb_t max_c(rgb_t a, rgb_t b) {
    if (b.r > a.r) a.r = b.r;
    if (b.g > a.g) a.g = b.g;
    if (b.b > a.b) a.b = b.b;
    return a;
}

static uint32_t clk, clk_rem, last_now;
static uint32_t dclk;
static uint32_t dreal;
static uint8_t  cur_id = 0xFF;
static bool     clk_started;

static void clock_step(uint32_t now, uint8_t speed) {
    uint32_t dt = clk_started ? now - last_now : 0;
    if (dt > 100) dt = 100;
    dreal       = dt;
    last_now    = now;
    clk_started = true;
    // speed scale the time step not the phase, so change speed never jump the animation
    clk_rem += dt * (32u + speed);
    dclk = clk_rem / 160;
    clk_rem %= 160;
    clk += dclk;
}

static uint16_t sm[N][3];

static void smooth_out(rgb_t out[N], uint16_t tau_ms, bool fresh) {
    uint32_t a = fresh ? 256 : dreal * 256 / (dreal + tau_ms);
    for (uint8_t i = 0; i < N; i++) {
        uint8_t *c[3] = {&out[i].r, &out[i].g, &out[i].b};
        for (uint8_t k = 0; k < 3; k++) {
            int32_t target = (int32_t)*c[k] << 8;
            sm[i][k]       = (uint16_t)(sm[i][k] + (target - (int32_t)sm[i][k]) * (int32_t)a / 256);
            *c[k]          = (uint8_t)((sm[i][k] + 128) >> 8 > 255 ? 255 : (sm[i][k] + 128) >> 8);
        }
    }
}

static const stop_t PAL_AURORA[] = {{0, 6, 12}, {0, 50, 40}, {10, 150, 80}, {0, 190, 140}, {10, 110, 190}, {70, 40, 190}, {160, 40, 160}, {230, 70, 120}};

static void fx_aurora(rgb_t out[N]) {
    for (uint8_t i = 0; i < N; i++) {
        uint32_t x   = led_x[i];
        uint8_t  n1  = noise2(x * 3 + clk / 16, clk / 48 + 0x10000);
        uint8_t  n2  = noise2(x * 5 + 0x8000 - clk / 11, clk / 30 + 0x40000);
        uint8_t  idx = clamp8(((int32_t)n1 - 40) * 255 / 175);
        uint8_t  c   = clamp8(((int32_t)n2 - 50) * 255 / 120);
        out[i]       = scale_c(pal(PAL_AURORA, 8, idx), 28 + scale8(sq8(c), 227));
    }
}

static const stop_t PAL_OCEAN[] = {{0, 4, 20}, {0, 14, 64}, {0, 44, 116}, {0, 96, 146}, {20, 150, 166}, {70, 200, 186}};

static void fx_ocean(rgb_t out[N]) {
    static const int16_t k[3]   = {230, 410, 150};
    static const int8_t  spd[3] = {9, -14, 5};
    static const uint8_t amp[3] = {150, 105, 125};
    uint8_t              thr    = 150 + (wave8(clk * 4) >> 3);
    for (uint8_t i = 0; i < N; i++) {
        uint32_t x = led_x[i];
        rgb_t    c = {.r = 0, .g = 3, .b = 12};
        for (uint8_t l = 0; l < 3; l++) {
            int32_t  wob = isin((uint16_t)(clk * (3 + l))) / 4;
            uint32_t ph  = x * (uint32_t)(int32_t)k[l] + clk * (uint32_t)(int32_t)spd[l] + (uint32_t)wob + l * 21000u;
            uint8_t  w   = wave8(ph);
            rgb_t    lc  = scale_c(pal(PAL_OCEAN, 6, w), scale8(sq8(w), amp[l]));
            c.r          = qadd8(c.r, lc.r);
            c.g          = qadd8(c.g, lc.g);
            c.b          = qadd8(c.b, lc.b);
        }
        uint8_t lum = (uint8_t)(((uint16_t)c.r + c.g + c.b) / 3);
        if (lum > thr) {
            uint8_t wc = clamp8((lum - thr) * 3);
            c.r        = qadd8(c.r, wc);
            c.g        = qadd8(c.g, wc);
            c.b        = qadd8(c.b, wc);
        }
        out[i] = c;
    }
}

#define TW_CYCLE 2600
#define TW_ATTACK 470
#define TW_DECAY 1610

static void fx_twinkle(rgb_t out[N], uint8_t hue, uint8_t sat) {
    rgb_t bg = scale_c(hsv_to_rgb((hsv_t){hue, sat, 255}), 14);
    for (uint8_t i = 0; i < N; i++) {
        uint32_t h   = hash32(i * 0x9E3779B1u ^ 0xB75u);
        uint32_t m   = 200 + (h & 0x7F);
        uint32_t ck  = (uint32_t)(((uint64_t)clk * m) >> 8) + (h >> 12);
        uint32_t cyc = ck / TW_CYCLE, ph = ck % TW_CYCLE;
        uint8_t  r   = hash32(i * 131u + cyc * 7919u) >> 24;
        rgb_t    c   = bg;
        if (r < 150) {
            uint8_t lvl = 0;
            if (ph < TW_ATTACK) {
                lvl = sq8(smooth8((uint8_t)(ph * 255 / TW_ATTACK)));
            } else if (ph < TW_ATTACK + TW_DECAY) {
                lvl = sq8((uint8_t)(255 - (ph - TW_ATTACK) * 255 / TW_DECAY));
            }
            uint8_t th = hue + (uint8_t)((r * 37u) & 15) - 8;
            uint8_t ts = (r & 3) == 0 ? (sat > 110 ? sat - 110 : 0) : sat;
            c          = max_c(bg, scale_c(hsv_to_rgb((hsv_t){th, ts, 255}), lvl));
        }
        out[i] = c;
    }
}

static const stop_t PAL_EMBER[] = {{8, 0, 0}, {60, 4, 0}, {130, 14, 0}, {200, 45, 0}, {240, 95, 5}, {255, 150, 30}, {255, 200, 90}};
#define EMBER_STEP 45
#define EMBER_FLOOR 38
static uint8_t  heat[N], heat_prev[N];
static uint16_t ember_acc;

static void ember_step(void) {
    uint8_t tmp[N];
    memcpy(heat_prev, heat, N);
    for (uint8_t i = 0; i < N; i++) {
        uint8_t c = rand8() % 14;
        heat[i]   = heat[i] > c ? heat[i] - c : 0;
    }
    for (uint8_t i = 0; i < N; i++) {
        uint8_t l = heat[i ? i - 1 : 0], r = heat[i < N - 1 ? i + 1 : N - 1];
        tmp[i]    = (uint8_t)(((uint16_t)l + 2 * heat[i] + r) / 4);
    }
    memcpy(heat, tmp, N);
    if (rand8() < 90) {
        uint8_t i = rand8() % N;
        heat[i]   = qadd8(heat[i], 70 + rand8() % 110);
    }
    for (uint8_t i = 0; i < N; i++) {
        if (heat[i] < EMBER_FLOOR) heat[i] = EMBER_FLOOR + (rand8() & 7);
    }
}

static void fx_ember(rgb_t out[N], bool fresh) {
    if (fresh) {
        for (uint8_t i = 0; i < N; i++) heat[i] = heat_prev[i] = 60 + (rand8() & 31);
        ember_acc = 0;
    }
    ember_acc += dclk;
    while (ember_acc >= EMBER_STEP) {
        ember_acc -= EMBER_STEP;
        ember_step();
    }
    uint8_t f = (uint8_t)(ember_acc * 255 / EMBER_STEP);
    uint8_t g = 215 + (noise2(clk / 3, 0x77000) >> 3);
    for (uint8_t i = 0; i < N; i++) {
        out[i] = scale_c(pal(PAL_EMBER, 7, lerp8(heat_prev[i], heat[i], f)), g);
    }
}

static const stop_t PAL_LAVA[] = {{16, 0, 30}, {60, 0, 72}, {140, 0, 92}, {210, 20, 52}, {250, 72, 10}, {255, 140, 22}, {255, 200, 80}};

static void fx_lava(rgb_t out[N]) {
    static const uint16_t period[3] = {13000, 19000, 27000};
    static const uint8_t  reach[3]  = {95, 80, 105};
    static const uint8_t  rad[3]    = {34, 28, 40};
    int32_t               cx[3], r2[3];
    for (uint8_t b = 0; b < 3; b++) {
        uint16_t ph = (uint16_t)((clk % period[b]) * 65536 / period[b] + b * 23000);
        cx[b]       = BAR_MID + (int32_t)reach[b] * isin(ph) / 32767;
        int32_t r   = rad[b] + (int32_t)isin((uint16_t)(clk * 3 + b * 17000)) * 6 / 32767;
        r2[b]       = r * r;
    }
    for (uint8_t i = 0; i < N; i++) {
        uint32_t f = 0;
        for (uint8_t b = 0; b < 3; b++) {
            int32_t  d  = led_x[i] - cx[b];
            uint32_t k  = (uint32_t)(r2[b] * 255 / (d * d + r2[b]));
            f          += k * k / 255;
        }
        out[i] = pal(PAL_LAVA, 7, clamp8(18 + (int32_t)f));
    }
}

#define BR_PERIOD 5600
#define BR_INHALE 2350

static void fx_breath(rgb_t out[N], uint8_t hue, uint8_t sat) {
    rgb_t base = hsv_to_rgb((hsv_t){hue, sat, 255});
    for (uint8_t i = 0; i < N; i++) {
        uint32_t lag = (uint32_t)abs32(led_x[i] - BAR_MID) * 2;
        uint32_t ph  = (clk + BR_PERIOD * 4 - lag) % BR_PERIOD;
        uint16_t s   = ph < BR_INHALE ? ease16(ph * 65535 / BR_INHALE) : 65535 - ease16((ph - BR_INHALE) * 65535 / (BR_PERIOD - BR_INHALE));
        out[i]       = scale_c(base, sq8((uint8_t)(64 + (uint32_t)s * 191 / 65535)));
    }
}

static void fx_colorwaves(rgb_t out[N]) {
    uint8_t  hue0   = (uint8_t)(clk / 38);
    uint32_t spread = 40 + (wave8((clk % 23000) * 65536 / 23000) >> 1);
    uint32_t k      = 200 + (wave8((clk % 17000) * 65536 / 17000) >> 1);
    for (uint8_t i = 0; i < N; i++) {
        uint8_t h   = hue0 + (uint8_t)(led_x[i] * spread / BAR_W);
        uint8_t b   = wave8(led_x[i] * k - clk * 12);
        uint8_t lvl = sq8((uint8_t)(110 + b * 145 / 255));
        out[i]      = scale_c(hsv_to_rgb((hsv_t){h, 235, 255}), lvl);
    }
}

#define RIP_MAX 8
#define RIP_LIFE 1300
typedef struct {
    uint32_t t0;
    uint8_t  x;
    bool     on;
} ripple_t;
static ripple_t ripples[RIP_MAX];
static uint8_t  rip_next;

static void fx_ripples(rgb_t out[N], uint8_t hue, uint8_t sat) {
    rgb_t    base = hsv_to_rgb((hsv_t){hue, sat, 255});
    uint16_t acc[N];
    memset(acc, 0, sizeof acc);
    for (uint8_t k = 0; k < RIP_MAX; k++) {
        ripple_t *rp = &ripples[k];
        if (!rp->on) continue;
        uint32_t age = clk - rp->t0;
        if (age >= RIP_LIFE) {
            rp->on = false;
            continue;
        }
        int32_t radius = (int32_t)age * 17 / 100;
        int32_t w      = 22 + (int32_t)age / 40;
        uint8_t amp    = scale8(sq8((uint8_t)(255 - age * 255 / RIP_LIFE)), 190);
        uint8_t core   = age < 300 ? sq8((uint8_t)(255 - age * 255 / 300)) : 0;
        for (uint8_t i = 0; i < N; i++) {
            int32_t d    = abs32(led_x[i] - rp->x);
            int32_t diff = abs32(d - radius);
            if (diff < w) acc[i] += scale8(sq8(clamp8(255 - diff * 255 / w)), amp);
            if (core && d < 20) acc[i] += scale8(core, (uint8_t)(255 - d * 255 / 20));
        }
    }
    for (uint8_t i = 0; i < N; i++) {
        uint8_t lvl = acc[i] > 255 ? 255 : (uint8_t)acc[i];
        rgb_t   c   = scale_c(base, lvl > 14 ? lvl : 14);
        if (acc[i] > 255) {
            uint8_t wc = clamp8((acc[i] - 255) / 2);
            c.r        = qadd8(c.r, wc);
            c.g        = qadd8(c.g, wc);
            c.b        = qadd8(c.b, wc);
        }
        out[i] = c;
    }
}

static uint16_t warm[N];

static void glow_hit(uint8_t x) {
    for (uint8_t i = 0; i < N; i++) {
        int32_t d = abs32(led_x[i] - x);
        if (d >= 36) continue;
        uint32_t add = 22000u * (uint32_t)(1296 - d * d) / 1296;
        warm[i]      = (uint16_t)(warm[i] + add > 65535 ? 65535 : warm[i] + add);
    }
}

static void fx_glow(rgb_t out[N], uint8_t hue, uint8_t sat) {
    static const rgb_t hot  = {.r = 255, .g = 190, .b = 140};
    rgb_t              base = hsv_to_rgb((hsv_t){hue, sat, 255});
    uint32_t           dk   = dclk > 40 ? 40 : dclk;
    for (uint8_t i = 0; i < N; i++) {
        warm[i]     = (uint16_t)(((uint32_t)warm[i] * (65536 - dk * 30)) >> 16);
        uint8_t c   = warm[i] >> 8;
        uint8_t q   = (uint8_t)(((uint16_t)sq8(c) + c) / 2);
        uint8_t lvl = 16 + scale8(q, 239);
        uint8_t wh  = c > 150 ? (uint8_t)((c - 150) * 150 / 105) : 0;
        out[i]      = scale_c(mix_c(base, hot, wh), lvl);
    }
}

bool bfx_is_reactive(uint8_t id) {
    return id == BFX_RIPPLES || id == BFX_GLOW;
}

void bfx_key_hit(uint8_t x) {
    if (cur_id == BFX_RIPPLES) {
        ripples[rip_next] = (ripple_t){.t0 = clk, .x = x, .on = true};
        rip_next          = (rip_next + 1) % RIP_MAX;
    } else if (cur_id == BFX_GLOW) {
        glow_hit(x);
    }
}

void bfx_render(uint8_t id, uint32_t now_ms, uint8_t hue, uint8_t sat, uint8_t speed, rgb_t out[N]) {
    bool fresh = id != cur_id;
    if (fresh) {
        cur_id = id;
        memset(ripples, 0, sizeof ripples);
        memset(warm, 0, sizeof warm);
    }
    clock_step(now_ms, speed);
    switch (id) {
        case BFX_AURORA:     fx_aurora(out); break;
        case BFX_OCEAN:      fx_ocean(out); break;
        case BFX_TWINKLE:    fx_twinkle(out, hue, sat); break;
        case BFX_EMBER:      fx_ember(out, fresh); smooth_out(out, 90, fresh); break;
        case BFX_LAVA:       fx_lava(out); break;
        case BFX_BREATH:     fx_breath(out, hue, sat); break;
        case BFX_COLORWAVES: fx_colorwaves(out); break;
        case BFX_RIPPLES:    fx_ripples(out, hue, sat); smooth_out(out, 35, fresh); break;
        case BFX_GLOW:       fx_glow(out, hue, sat); smooth_out(out, 70, fresh); break;
        default:             memset(out, 0, sizeof(rgb_t) * N); break;
    }
}
