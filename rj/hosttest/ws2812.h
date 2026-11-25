#pragma once
#include <stdint.h>
typedef struct { uint8_t r, g, b; } ws2812_led_t;
void ws2812_set_color(int index, uint8_t r, uint8_t g, uint8_t b);
void ws2812_flush(void);
