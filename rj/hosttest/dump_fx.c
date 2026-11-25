#include <stdio.h>
#include <string.h>
#include "bar_fx.h"

rgb_t hsv_to_rgb(hsv_t h) {
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

static const char B64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static void b64(const uint8_t *d, size_t n) {
    for (size_t i = 0; i < n; i += 3) {
        uint32_t v = d[i] << 16 | (i + 1 < n ? d[i + 1] << 8 : 0) | (i + 2 < n ? d[i + 2] : 0);
        putchar(B64[v >> 18 & 63]); putchar(B64[v >> 12 & 63]);
        putchar(i + 1 < n ? B64[v >> 6 & 63] : '='); putchar(i + 2 < n ? B64[v & 63] : '=');
    }
}

#define STEP 33
#define DUR 14000
static uint8_t buf[(DUR / STEP + 1) * 42];

static uint32_t r = 12345;
static uint32_t rnd(uint32_t n) { r = r * 1103515245u + 12345u; return (r >> 16) % n; }

int main(int argc, char **argv) {
    const uint8_t presets[][2] = {{170, 255}, {200, 255}, {0, 255}};
    uint32_t clock = 5000;
    printf("{\"step\":%d,\"fx\":{", STEP);
    for (uint8_t id = 0; id < BFX_COUNT; id++) {
        bool colour = id == BFX_TWINKLE || id == BFX_BREATH || id == BFX_RIPPLES || id == BFX_GLOW;
        printf("%s\"%u\":[", id ? "," : "", id);
        for (uint8_t p = 0; p < (colour ? 3 : 1); p++) {
            r = 777;
            uint32_t next_key = 600, word_left = 4;
            size_t n = 0;
            for (uint32_t t = 0; t <= DUR - STEP; t += STEP) {
                if (bfx_is_reactive(id)) {
                    while (next_key <= t) {
                        if (word_left == 0) {
                            bfx_key_hit(94);
                            word_left = 3 + rnd(5);
                            next_key += rnd(10) == 0 ? 900 + rnd(600) : 160 + rnd(80);
                        } else {
                            bfx_key_hit((uint8_t)(28 + rnd(170)));
                            word_left--;
                            next_key += 85 + rnd(90);
                        }
                    }
                }
                rgb_t out[BFX_LEDS];
                bfx_render(id, clock + t, presets[p][0], presets[p][1], 128, out);
                bfx_render(id, clock + t + 16, presets[p][0], presets[p][1], 128, out);
                for (int i = 0; i < BFX_LEDS; i++) { buf[n++] = out[i].r; buf[n++] = out[i].g; buf[n++] = out[i].b; }
            }
            clock += DUR + 1000;
            printf("%s\"", p ? "," : "");
            b64(buf, n);
            printf("\"");
        }
        printf("]");
        bfx_render(0xFE, clock, 0, 0, 128, (rgb_t[BFX_LEDS]){0});
    }
    printf("}}\n");
    return 0;
}
