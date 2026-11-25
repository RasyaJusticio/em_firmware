#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "keycodes.h"
#include "quantum_keycodes_legacy.h"

#define PROGMEM
#define MATRIX_ROWS 6
#define MATRIX_COLS 15
#define _______ KC_TRNS
#define MO(layer) (QK_MOMENTARY | ((layer)&0x1F))
#define LAYOUT_ansi(...) {__VA_ARGS__}

enum keyboard_keycodes { BT_HOST1 = QK_KB_0, BT_HOST2, BT_HOST3, BT_2_4G, BT_USB, BT_SW, BT_VOL, LOW_D, COM_D, HIGH_D, USBSLP, RGB_TEST };

#define LED_ESC_INDEX 0
#define LED_F1_INDEX 1
#define LED_F12_INDEX 12
#define LED_DEL_INDEX 13
#define EECONFIG_USER_DATA_SIZE 32
#define BT_CABLE_PIN 1
#define BT_CHARGE_PIN 2

typedef struct { struct { struct { uint8_t row, col; } key; bool pressed; } event; } keyrecord_t;
typedef uint32_t layer_state_t;
typedef union { uint8_t raw; struct { bool num_lock : 1; bool caps_lock : 1; }; } led_t;

extern layer_state_t layer_state, default_layer_state;
void     default_layer_set(layer_state_t state);
uint32_t timer_read32(void);
uint32_t timer_elapsed32(uint32_t last);
void     register_code(uint8_t kc);
void     unregister_code(uint8_t kc);
void     rgb_matrix_set_color(int index, uint8_t r, uint8_t g, uint8_t b);
led_t    host_keyboard_led_state(void);
bool     gpio_read_pin(int pin);
bool     eeconfig_is_user_datablock_valid(void);
void     eeconfig_read_user_datablock(void *data);
void     eeconfig_update_user_datablock(const void *data);

bool    rgb_matrix_is_enabled(void);
uint8_t rgb_matrix_get_val(void);
uint8_t rgb_matrix_get_hue(void);
uint8_t rgb_matrix_get_sat(void);
void    rgb_matrix_sethsv_noeeprom(uint16_t h, uint8_t s, uint8_t v);

#define RGB_MATRIX_MAXIMUM_BRIGHTNESS 200
#define EECONFIG_USER ((uint32_t *)0x10)
#define EECONFIG_USER_DATABLOCK ((uint8_t *)0x20)
uint32_t last_input_activity_elapsed(void);
void     wait_ms(uint32_t ms);
uint8_t  rgb_matrix_get_speed(void);
void     rgb_matrix_sethsv(uint16_t h, uint8_t s, uint8_t v);
void     rgb_matrix_set_speed(uint8_t speed);
void     rgb_matrix_increase_val_noeeprom(void);
void     rgb_matrix_decrease_val_noeeprom(void);
void     rgb_matrix_increase_hue_noeeprom(void);
void     rgb_matrix_decrease_hue_noeeprom(void);
void     rgb_matrix_increase_sat_noeeprom(void);
void     rgb_matrix_decrease_sat_noeeprom(void);
void     rgb_matrix_increase_speed_noeeprom(void);
void     rgb_matrix_decrease_speed_noeeprom(void);
uint32_t eeprom_read_dword(const uint32_t *addr);
void     eeprom_read_block(void *buf, const void *addr, uint32_t len);
void     rgb_matrix_set_speed_noeeprom(uint8_t speed);

enum rgb_matrix_effects { RGB_MATRIX_NONE, RGB_MATRIX_SOLID_COLOR, RGB_MATRIX_CYCLE_ALL, RGB_MATRIX_CYCLE_LEFT_RIGHT, RGB_MATRIX_CYCLE_OUT_IN,
    RGB_MATRIX_DUAL_BEACON, RGB_MATRIX_RAINBOW_BEACON, RGB_MATRIX_CUSTOM_BAR_AURORA, RGB_MATRIX_CUSTOM_BAR_OCEAN, RGB_MATRIX_CUSTOM_BAR_TWINKLE,
    RGB_MATRIX_CUSTOM_BAR_EMBER, RGB_MATRIX_CUSTOM_BAR_LAVA, RGB_MATRIX_CUSTOM_BAR_BREATH, RGB_MATRIX_CUSTOM_BAR_COLORWAVES,
    RGB_MATRIX_CUSTOM_BAR_RIPPLES, RGB_MATRIX_CUSTOM_BAR_GLOW, RGB_MATRIX_EFFECT_MAX };
uint8_t  rgb_matrix_get_mode(void);
void     rgb_matrix_mode(uint8_t mode);
uint16_t dynamic_keymap_get_keycode(uint8_t layer, uint8_t row, uint8_t col);
void     dynamic_keymap_set_keycode(uint8_t layer, uint8_t row, uint8_t col, uint16_t keycode);
