# Disable default low power implementation, use lp_sleep.c style instead
WIRELESS_LPWR_STOP_ENABLE = no
ENTRY_STOP_MODE_ENABLE = yes

VIA_ENABLE = yes

SRC += wake_replay.c
OPT_DEFS += -DBRIDGE75_WAKE_REPLAY

include keyboards/shortcut/wireless/wireless.mk
