#pragma once
typedef struct { uint8_t h, s, v; } hsv_t;
typedef struct { uint8_t r, g, b; } rgb_t;
rgb_t hsv_to_rgb(hsv_t hsv);
