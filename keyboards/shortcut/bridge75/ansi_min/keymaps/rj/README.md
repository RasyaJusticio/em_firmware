# Bridge75: Jscio Edition

RJ builds, based on emolitor/qmk_firmware `em-2.0.0`. Target: `shortcut/bridge75/ansi_min`, keymap `rj`.
Releases are tagged `rj-<version>` on the `RJ` branch.

`ansi_min` is the stock ANSI board with only the first LED row (Esc, F1–F12, Del = 14 LEDs)
mapped, because the LED chain on this board is broken after the first row. The LEDs after
LED 14 are never sent data.

## Build

```sh
qmk compile -kb shortcut/bridge75/ansi_min -km rj
```

Output: `shortcut_bridge75_ansi_min_rj.bin`

Pushing a tag `rj-<version>` builds it on GitHub (`.github/workflows/rj-build.yml`), runs the
host tests and publishes a release with the .bin, the VIA definition and this README.

## Development

Everything besides the firmware lives in `rj/` at the repo root:

* `rj/test.sh`: host tests. They compile the real `keymap.c`, `bar_fx.c`, `startup_anim.c` and
  the keyboard level modules against small QMK stubs (`rj/hosttest/`), with plain gcc.
* `rj/gen_via_json.py keyboards/shortcut/bridge75/ansi_min/keyboard.json <out.json>`: the VIA
  definition. Keep its effect list in step with `rgb_matrix_user.inc` (a static assert in
  `keymap.c` fails the build when the numbers move).
* `rj/preview/`: the light preview page. `dump_fx.c` / `dump_frames.c` in `rj/hosttest/` render
  the real effects and startup animations to `fx.json` / `anims.json`.
* `rj/strip_comments.py`: code style is no comments unless one is really needed.

## Flash

1. Hold **Esc** while plugging the board in -> WB32 DFU mode.
2. `wb32-dfu-updater_cli -D shortcut_bridge75_ansi_min_rj.bin` then `wb32-dfu-updater_cli -R`
   (or QMK Toolbox: open the .bin -> Flash -> Exit DFU).

The vendor guide also has you switch to cable mode (FN+`~`) and turn the battery off first.
Holding Esc works on its own for this board; fall back to those steps if DFU mode doesn't show up.

After flashing, the settings initialise themselves. VIA keymaps are reset by QMK
whenever the settings layout changes.

## VIA

VIA → Settings → enable *Show Design tab* → Design → *Load Draft Definition* →
`bridge75_ansi_min_rj_via.json`. The VID/PID are the same as stock (0xFFFE / 0x003E).

Names: USB / VIA show **Bridge75: Jscio Edition**. Bluetooth shows **Bridge75 Jscio BT1/2/3**
(the wireless module caps Bluetooth names at 18 characters). Already-paired computers keep the
old name until you re-pair.

You get:

* **Lighting**: brightness, effect, speed and colour for the F-row effect.
* **Bridge75 Settings**: active profile, profile colours, when Delete shows the profile
  colour, the WASD SOCD mode, effect lights on/off, the startup animation, and the hard
  RGB idle cutoff.
* **Custom keycodes** (Keymap → Custom): BT1–3, 2.4G, USB, Cable/2.4G, Sleep, Prof+, Public,
  Personal, Gaming, FX Tog, Play Anim.

VIA shows 6 layers: 0 / 1 / 2 = Public / Personal / Gaming base, 3 / 4 / 5 = their FN layers.
Each base layer's FN key must stay `MO(own FN layer)`: MO(3) on layer 0, MO(4) on 1, MO(5) on 2.
The FN layers sit above all base layers on purpose, so don't move them.

## Keys

| Key | Public | Personal | Gaming |
|---|---|---|---|
| Caps position | Caps Lock | Left Ctrl | Left Ctrl |
| Bottom-left Ctrl | Left Ctrl | Caps Lock | Caps Lock |
| FN + H J K L | – | ← ↓ ↑ → | ← ↓ ↑ → |
| W A S D | normal | normal | SOCD (default: last input wins) |

FN layer (all profiles):

| FN + | Action | FN + | Action |
|---|---|---|---|
| F1 / F2 | screen brightness − / + | F9 / F10 | RGB brightness − / + |
| F3 / F4 | volume − / + | F11 | effect lights on/off (indicators stay) |
| F5 | mute | F12 | Print Screen |
| F6 / F7 / F8 | prev / play-pause / next | Delete | Insert |
| Home | next profile | End | soft reboot (`QK_REBOOT`) |
| `~` | cable ⇄ 2.4G dongle (from Bluetooth: cable if plugged in, else dongle; hold 3 s when it switches to the dongle to pair it) | `\` | keep-awake toggle |
| 1 2 3 | BT1 / BT2 / BT3 (hold 3 s to pair) | 4 | 2.4G dongle (hold 3 s to pair) |
| Z / X | previous / next effect | C / V | colour back / forward (hold to sweep) |
| B / N | slower / faster (hold to repeat) | | |

Removed from stock: FN+Esc EEPROM wipe (still available as a VIA keycode), the Win/Mac
layer switch, and the Mac layers.

## Lights

| LED | Normal | FN held |
|---|---|---|
| Esc | white = Caps Lock on | battery: green > 90 %, blue > 50 %, yellow > 10 %, breathing red ≤ 10 %, white = charging |
| F5–F8 | blinks if the wireless link is down | connection, in the same order as FN + `~` 1 2 3: F5 = USB cable (orange) or 2.4G dongle (green), F6/F7/F8 = BT1/BT2/BT3 (blue). Active slot lit, others dim; slow breathing = disconnected, fast breathing = pairing, one slow swell = just connected |
| F11 | – | red = keep-awake (FN+`\`) is on: no RGB timeouts and no deep sleep, so the battery drains |
| Delete | profile colour for 2 s after switching (or always, see VIA); right after the switch sweep passes it, it flickers twice and settles | profile colour (Public blue, Personal purple, Gaming red by default) |

The battery reading comes from the wireless module. On USB, or for the first few seconds
after boot or wake, it reports 100 % until the module answers. Treat it as approximate.

**Startup animations** (VIA → Bridge75 Settings → Startup Animation):

| Animation | Looks like |
|---|---|
| Welcome (default, in #00FCFF) | car light-bar welcome: a soft-edged fill sweeps across, lifts, then sweeps off |
| Ignition | two sparks race in from the ends, meet, and bloom over the whole bar |
| Sunrise | deep red to warm white, the middle rising first, then it sets |
| Comets | two white comets meet in the middle and run back out |
| Curtain | profile colour fills from the centre, then opens |
| Rainbow Wipe | rainbow wipes in left to right, then out |
| Larson Scanner | red Knight Rider eye sweeping back and forth |
| Meteor | icy meteor with a breaking-up tail |
| Fire | flames ignite in the middle and burn out |
| Power Up | red-to-green charge bar, white flash |
| Ripple | drops landing and rippling outwards |
| Bounce | two balls bouncing to rest at each end |
| Heartbeat | red lub-dub pulse from the centre |
| Shuffle | the next one in the list every power-up |

Welcome, Ignition and Sunrise replaced Confetti, Theater Chase and Juggle (busy demo patterns
that looked cheap on 14 LEDs). A board set to one of those plays its replacement.

**Light effects** (VIA → Lighting → Effect). The effects that don't work on a single row are
gone (Cycle Up/Down and Rainbow Chevron were copies of other effects, Pinwheel / Spiral / Out-In
Dual were near copies, Raindrops popped, and Heatmap / Reactive / Multisplash only reacted to
F-key presses). New ones, written for this bar:

| Effect | Looks like |
|---|---|
| Aurora | green, teal and violet curtains drifting across the bar |
| Ocean (default, speed 191 = 75 %) | layered blue-green swells with white caps |
| Twinkle | keys glow in and fade out at random, each on its own clock (VIA colour) |
| Ember | a bed of coals: heat drifts and flares, never burns out |
| Lava | three warm blobs on slow orbits that merge and split |
| Calm Breath | 5.6 s breath from the centre, never fully dark (VIA colour) |
| Colorwaves | hue and brightness waves drifting against each other |
| Typing Ripples | every key press sends a ring out from the spot above it (VIA colour) |
| Typing Glow | typing warms the bar above your hands, cools when you stop (VIA colour) |

Still there: Solid Color, Cycle All, Cycle Left/Right, Cycle Out/In, Dual Beacon, Rainbow
Beacon. The first boot after flashing moves a saved effect that no longer exists to the closest
one (Breathing → Calm Breath, Raindrops → Twinkle, Heatmap → Typing Glow, reactive ones →
Typing Ripples). Speed works on all of them and never makes the animation jump.

* **Colour:** Auto (each animation's own), your profile colour, a custom colour, or rainbow.
* **When:** only on a real power-up, which means the battery switch going on, or USB
  plugged in with the battery off or flat. Not on wake, soft reboot, or a normal plug-in
  while the battery is on.
* **Preview:** the VIA *Preview* button or the `Play Anim` custom key.
* **Afterwards:** when the effect lights are on, the F-row effect fades in from black to
  your brightness over 0.9 s on an eased curve. Pressing an RGB key during the animation
  cancels the fade and uses your real brightness.

## How the lights behave

* **Nothing snaps.** Indicators fade in (0.12 s) and out (0.22 s), and the effect fades when
  toggled with FN+F11. Caps Lock is direct feedback, so it gets a quick attack (0.04 s) and a
  soft release (0.16 s). The LEDs refresh at about 60 fps.
* **Fades follow the eye.** LED brightness is linear, the eye isn't, so a plain fade looks like
  it hangs and then drops off. Every fade (indicators, FN dim, F11, idle dim, goodbye, the
  startup fade-in) runs on a perceptual curve instead. End points are unchanged.
* **One effect brightness.** FN+F11 and the FN dim feed a single eased brightness, so letting
  go of FN in the middle of an F11 fade bends the fade instead of jumping.
* **Changing effect crossfades** over 0.4 s instead of cutting.
* **Switching to wired** restarts USB, which QMK reports as a wake-up. The lights ignore it
  (only a real sleep, where they faded and powered off, fades them back in), so nothing blinks.
* **Holding FN** dims the F-row effect to 25 % so the indicators stand out. Adjust this in VIA
  (100 % = no dimming).
* **Brightness HUD (live preview):** FN+F9/F10 turns the F-row into a preview of your real
  effect at the new brightness. F1 up to your level shows it exactly as it will look, the rest
  goes dark, and every other light (and the FN dimming) steps aside until about 1 s after you
  let go. Hold a key to ramp smoothly; brightness has 25 steps. Speed works the same way. For
  hue and saturation the whole row shows the live effect. The preview also works with the
  effect lights switched off.
* **Profile switch:** a sweep in the new profile's colour runs across the row.
* **Navigation focus (FN + HJKL):** while you move around with the FN arrows, every FN light
  hides and only the pressed arrow glows softly on the matching key: H/J/K/L -> F5/F6/F7/F8.
  Focus holds about 0.7 s between strokes and ends as soon as FN is released. In VIA you can
  set the effect while navigating (dimmed like FN, normal, or off), turn the echo off, or
  turn the whole thing off. Trigger keys and echo keys are one table (`nav_keys[]` in
  keymap.c).
* **Idle:** after 30 s without input the lights ease down to 30 %. At 58–60 s they fade out
  completely, before the 61 s timeout. The next key press brings them back over 0.2 s.
* **PC sleep:** the lights fade out over 0.6 s before power is cut, and ease back in on wake.
* Every one of these can be switched off or tuned in VIA → Bridge75 Settings → Light Behaviour.

## Mash guard

With the board face down, or something resting on it, FN and lots of keys are held at once.
Mode and settings keys (connection, pairing, reboot, keep-awake, profiles, RGB, EEPROM) only
act when FN plus that one key are held. If a connection switch slips through as keys land,
it's undone when the key is released. Normal typing and rollover are unaffected.

## Power / RGB timeouts

* Stock behaviour, unchanged: RGB turns off after 60 s idle, and LED power is cut after
  about 2.6 min idle on USB. On battery the board deep-sleeps after 5 min.
* FN + `\` (keep-awake) disables those timeouts. Added here: a hard cutoff, 30 min after
  the last key press by default, that cuts LED power even with keep-awake on. Set it in VIA
  (0 = never).
* Unplugging the cable in cable mode is treated as unplugged, not as the computer sleeping.
  The lights keep behaving normally (dim at 30 s, fade out at 60 s, deep sleep at 5 min);
  only a real PC sleep with the cable connected fades them out early.
* Any key press brings the lights back after a timeout, and that key is still typed.
* Waking from battery deep sleep resets the chip, and the wireless link takes 1–3 s to come
  back. Keys typed in the first 4 s are kept and sent once the link is up, so the start of
  what you type isn't lost. (The very first key can still be missed if it is released before
  the chip finishes booting.) Toggle: VIA → Lights & Power.

## Notes

* The VIA effect "All Off" (or an `RGB_TOG` key you map yourself) turns the whole RGB matrix off, indicators
  included. To keep the indicators with no effect, use FN+F11 or brightness 0 instead.
* If you keep FN held after FN+Home, the old profile's FN layer stays active, so FN+Home
  again keeps cycling. The new profile's FN extras (e.g. HJKL arrows) apply from the next
  FN press.
* Host-side logic tests: `tools/hosttest` (`test_keymap.c`, `test_kb_modules.c`; build lines at the top of each).
