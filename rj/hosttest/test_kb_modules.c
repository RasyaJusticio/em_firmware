#include <stdio.h>
#include <string.h>
#include "keycodes.h"
#include "quantum_keycodes_legacy.h"
#include "mash_guard.h"
#include "wake_replay.h"

enum { BT_HOST1 = QK_KB_0, USBSLP = QK_KB_0 + 10, PRF_NXT = QK_KB_0 + 12, ANIM_PLAY = QK_KB_0 + 17 };
#define MO(layer) (QK_MOMENTARY | ((layer)&0x1F))
#define PDF(layer) (QK_PERSISTENT_DEF_LAYER | ((layer)&0x1F))
#define LCTL(kc) (0x0100 | (kc))
#define LSFT(kc) (0x0200 | (kc))
#define KC_LPRN LSFT(KC_9)
#define LCTL_T(kc) (QK_MOD_TAP | (0x01 << 8) | ((kc)&0xFF))

static int failures, checks;
#define CHECK(cond, ...) do { checks++; if (!(cond)) { failures++; printf("FAIL %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

static report_keyboard_t sent[32];
static int               nsent;
static void              capture_send(report_keyboard_t *r) { if (nsent < 32) sent[nsent] = *r; nsent++; }
static report_keyboard_t rep_key(uint8_t k) { report_keyboard_t r; memset(&r, 0, sizeof r); r.keys[0] = k; return r; }

int main(void) {
    const uint16_t guarded[] = {BT_HOST1, USBSLP, PRF_NXT, ANIM_PLAY, RGB_TOG, RGB_VAI, RGB_MOD, QK_REBOOT, QK_BOOT, QK_CLEAR_EEPROM, NK_TOGG, PDF(1)};
    const uint16_t free_keys[] = {KC_A, KC_SPC, KC_ESC, KC_LPRN, LCTL(KC_C), MO(3), KC_VOLU, KC_MPLY, KC_LEFT, KC_PSCR, KC_INS, KC_LSFT, LCTL_T(KC_A), KC_TRNS};
    for (unsigned i = 0; i < sizeof guarded / sizeof *guarded; i++) CHECK(mash_guarded_keycode(guarded[i]), "0x%04X must be guarded", guarded[i]);
    for (unsigned i = 0; i < sizeof free_keys / sizeof *free_keys; i++) CHECK(!mash_guarded_keycode(free_keys[i]), "0x%04X must never be blocked", free_keys[i]);
    CHECK(!mash_guard_blocks(BT_HOST1, true, 2), "FN + BT1 alone works");
    CHECK(mash_guard_blocks(BT_HOST1, true, 3), "BT1 with extra keys held is blocked");
    CHECK(mash_guard_blocks(RGB_TOG, true, 40), "face down: RGB toggle blocked");
    CHECK(!mash_guard_blocks(BT_HOST1, false, 40), "releases are never blocked");
    CHECK(!mash_guard_blocks(KC_A, true, 40), "typing with many keys held (rollover) is never blocked");

    CHECK(mash_latch_next(false, 3) && mash_latch_next(true, 2) && !mash_latch_next(true, 1) && !mash_latch_next(true, 0),
          "latch rules");
    CHECK(!mash_latch_next(false, 2), "FN + key alone never latches");

    report_keyboard_t a = rep_key(KC_A), none = rep_key(0), b = rep_key(KC_B);
    CHECK(!wake_replay_capture(&a, 0), "not armed: nothing kept");

    wake_replay_arm(1000);
    CHECK(wake_replay_capture(&a, 1200) && wake_replay_capture(&none, 1260) && wake_replay_capture(&b, 1400), "keys typed while reconnecting are kept");
    nsent = 0;
    CHECK(wake_replay_poll(false, 2000, capture_send) == 0 && nsent == 0, "nothing sent while still disconnected");
    CHECK(wake_replay_poll(true, 2600, capture_send) == 3 && nsent == 3, "replayed once connected");
    CHECK(sent[0].keys[0] == KC_A && sent[1].keys[0] == 0 && sent[2].keys[0] == KC_B, "replayed in order");
    CHECK(!wake_replay_armed() && !wake_replay_capture(&a, 2700), "disarmed after replay");
    CHECK(wake_replay_poll(true, 2800, capture_send) == 0, "nothing twice");

    wake_replay_arm(0);
    wake_replay_capture(&a, 100);
    CHECK(!wake_replay_capture(&b, 4000), "nothing kept after the window");
    CHECK(wake_replay_poll(false, 4100, capture_send) == 0 && !wake_replay_armed(), "window ends: stale keys dropped");
    nsent = 0;
    CHECK(wake_replay_poll(true, 4200, capture_send) == 0 && nsent == 0, "late connection types nothing");

    wake_replay_arm(0);
    for (int k = 0; k < 20; k++) { report_keyboard_t r = rep_key(KC_A + k); wake_replay_capture(&r, 10 + k); }
    nsent = 0;
    CHECK(wake_replay_poll(true, 500, capture_send) == WAKE_REPLAY_MAX, "overflow: capped at %d", WAKE_REPLAY_MAX);
    CHECK(sent[WAKE_REPLAY_MAX - 1].keys[0] == KC_A + 19, "overflow: the final state is still sent last");

    wake_replay_arm(0xFFFFFF00u);
    CHECK(wake_replay_capture(&a, 0x00000010u), "timer wraparound inside the window");

    printf("%d/%d checks passed\n", checks - failures, checks);
    return failures ? 1 : 0;
}
