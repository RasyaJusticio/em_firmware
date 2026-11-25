#include <stdio.h>
#include "startup_anim.h"

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

int main(void) {
    hsv_t profile = {170, 255, 255};
    hsv_t custom  = {128, 255, 255};
    printf("{\"step\":33,\"level\":%d,\"anims\":{", ANIM_LEVEL);
    for (uint8_t id = ANIM_FIRST_REAL; id <= ANIM_LAST_REAL; id++) {
        printf("%s\"%u\":{\"dur\":%u,\"modes\":[", id == ANIM_FIRST_REAL ? "" : ",", id, anim_duration(id));
        for (uint8_t m = 0; m < ANIM_COLOR_COUNT; m++) {
            printf("%s[", m ? "," : "");
            anim_start(id, m, profile, custom, 0xB75u + id);
            rgb_t out[ANIM_LEDS];
            for (uint32_t t = 0, n = 0; anim_render(t, out); t += 33, n++) {
                printf("%s\"", n ? "," : "");
                for (int i = 0; i < ANIM_LEDS; i++) printf("%02x%02x%02x", out[i].r, out[i].g, out[i].b);
                printf("\"");
            }
            printf("]");
        }
        printf("]}");
    }
    printf("}}\n");
    return 0;
}
