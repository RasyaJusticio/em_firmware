#pragma once
enum { DEVS_USB = 0, DEVS_BT1, DEVS_BT2, DEVS_BT3, DEVS_BT4, DEVS_BT5, DEVS_2G4 };
enum { MD_STATE_NONE = 0, MD_STATE_PAIRING, MD_STATE_CONNECTED, MD_STATE_DISCONNECTED, MD_STATE_REJECT };
uint8_t *md_getp_state(void);
uint8_t *md_getp_bat(void);
uint8_t  wireless_get_current_devs(void);
