#pragma once
#define BRIDGE75_RGB_HARD_TIMEOUT_MIN 30
extern bool     bridge75_cold_boot;
extern uint16_t bridge75_rgb_hard_timeout_min;
bool bridge75_keep_awake(void);
#define BRIDGE75_WAKE_REPLAY
extern bool bridge75_wake_replay;
bool bridge75_mash_active(void);
