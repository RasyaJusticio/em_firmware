#!/bin/sh
set -e
cd "$(dirname "$0")/.."
OUT=$(mktemp -d)
KM=keyboards/shortcut/bridge75/ansi_min/keymaps/rj
KB=keyboards/shortcut/bridge75
CC=${CC:-gcc}
$CC -std=gnu11 -Wall -Wextra -Wno-missing-braces -Wno-unused-parameter -Irj/hosttest -Iquantum -DWIRELESS_ENABLE \
    -DQMK_KEYBOARD_H='"qmk_stub.h"' rj/hosttest/test_keymap.c $KM/startup_anim.c $KM/bar_fx.c -o "$OUT/test_keymap"
$CC -std=gnu11 -Wall -Wextra -Irj/hosttest -Iquantum -I$KB rj/hosttest/test_kb_modules.c $KB/wake_replay.c -o "$OUT/test_kb_modules"
"$OUT/test_keymap" | tail -1
"$OUT/test_kb_modules" | tail -1
"$OUT/test_keymap" | grep -q "^FAIL" && exit 1
"$OUT/test_kb_modules" | grep -q "^FAIL" && exit 1
rm -rf "$OUT"
