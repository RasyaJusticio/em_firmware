// Copyright 2023 JoyLee (@itarze)
// Copyright 2025 emolitor (github.com/emolitor)
// SPDX-License-Identifier: GPL-2.0-or-later

#ifdef NO_USB_STARTUP_CHECK

#include <ch.h>
#include <hal.h>

#include "usb_main.h"

/* TMK includes */
#include "action_util.h"
#include "mousekey.h"
#include "print.h"

#include "suspend.h"
#include "wait.h"

#include "quantum.h"
#include "wireless.h"

static bool usb_cable_present(void) {
#ifdef BT_CABLE_PIN
    return gpio_read_pin(BT_CABLE_PIN);
#else
    return true;
#endif
}

void housekeeping_task_usb_suspend(void) {

    // cable mode with no cable look same as host sleep, only trust suspend when cable is in
    if (wireless_get_current_devs() == DEVS_USB && usb_cable_present()) {
        if ((USB_DRIVER.state == USB_SUSPENDED) && (USB_DRIVER.saved_state == USB_ACTIVE)) {
            print("[s]");
            while (USB_DRIVER.state == USB_SUSPENDED) {
                if (!usb_cable_present()) {
                    suspend_wakeup_init();
                    return;
                }
                /* Do this in the suspended state */
                suspend_power_down(); // on AVR this deep sleeps for 15ms
                /* Remote wakeup */
                if (suspend_wakeup_condition()) {
                    usbWakeupHost(&USB_DRIVER);
                    restart_usb_driver(&USB_DRIVER);
                }
            }
            /* Woken up */
            // variables has been already cleared by the wakeup hook
            send_keyboard_report();
#ifdef MOUSEKEY_ENABLE
            mousekey_send();
#endif /* MOUSEKEY_ENABLE */
        }
    }
}
#endif
