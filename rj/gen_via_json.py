import json
import sys

src, out = sys.argv[1], sys.argv[2]
kb = json.load(open(src))
info_layout = kb["layouts"]["LAYOUT_ansi"]["layout"]

rows = {}
for key in info_layout:
    rows.setdefault(key["y"], []).append(key)

kle = []
prev_y = -1.0
for y in sorted(rows):
    row = []
    gap_y = y - prev_y - 1
    pending = {}
    if gap_y > 0.001:
        pending["y"] = round(gap_y, 4)
    cursor = 0.0
    for key in sorted(rows[y], key=lambda k: k["x"]):
        props = dict(pending)
        pending = {}
        dx = key["x"] - cursor
        if dx > 0.001:
            props["x"] = round(dx, 4)
        w = key.get("w", 1)
        if w != 1:
            props["w"] = w
        if props:
            row.append(props)
        row.append("%d,%d" % tuple(key["matrix"]))
        cursor = key["x"] + w
    kle.append(row)
    prev_y = y

custom = [
    ("BT1", "Bluetooth host 1 (hold 3s to pair)"),
    ("BT2", "Bluetooth host 2 (hold 3s to pair)"),
    ("BT3", "Bluetooth host 3 (hold 3s to pair)"),
    ("2.4G", "2.4 GHz dongle (hold 3s to pair)"),
    ("USB", "Wired / USB mode"),
    ("Cable/2.4G", "Toggle wired / 2.4 GHz dongle (hold 3s when switching to the dongle to pair it)"),
    ("BT_VOL", "Unused"),
    ("LOW_D", "Unused"),
    ("COM_D", "Unused"),
    ("HIGH_D", "Unused"),
    ("Sleep", "Toggle keep-awake (disables RGB/sleep timeouts, hard RGB cutoff still applies)"),
    ("RGB_TEST", "Unused"),
    ("Prof+", "Next profile"),
    ("Public", "Switch to Public profile"),
    ("Personal", "Switch to Personal profile"),
    ("Gaming", "Switch to Gaming profile"),
    ("FX Tog", "Toggle effect lights on the first row (indicators stay)"),
    ("Play Anim", "Play the startup animation now (preview)"),
]
custom_keycodes = [{"name": n, "title": t, "shortName": n} for n, t in custom]

effects = [
    ["All Off (hides indicators too)", 0], ["Solid Color", 1], ["Cycle All", 2], ["Cycle Left/Right", 3],
    ["Cycle Out/In", 4], ["Dual Beacon", 5], ["Rainbow Beacon", 6],
    ["Aurora", 7], ["Ocean", 8], ["Twinkle", 9], ["Ember", 10], ["Lava", 11], ["Calm Breath", 12],
    ["Colorwaves", 13], ["Typing Ripples", 14], ["Typing Glow", 15],
]
color_effects = [1, 9, 12, 14, 15]

lighting = {
    "label": "Lighting",
    "content": [{
        "label": "Effect (F1-F12 row)",
        "content": [
            {"label": "Brightness", "type": "range", "options": [0, 255], "content": ["id_qmk_rgb_matrix_brightness", 3, 1]},
            {"label": "Effect", "type": "dropdown", "content": ["id_qmk_rgb_matrix_effect", 3, 2], "options": effects},
            {"showIf": "{id_qmk_rgb_matrix_effect} != 0 && {id_qmk_rgb_matrix_effect} != 1", "label": "Effect Speed", "type": "range",
             "options": [0, 255], "content": ["id_qmk_rgb_matrix_effect_speed", 3, 3]},
            {"showIf": " || ".join("{id_qmk_rgb_matrix_effect} == %d" % e for e in color_effects),
             "label": "Color", "type": "color", "content": ["id_qmk_rgb_matrix_color", 3, 4]},
        ],
    }],
}

CH = 0
settings = {
    "label": "Bridge75 Settings",
    "content": [
        {
            "label": "Profiles",
            "content": [
                {"label": "Active profile", "type": "dropdown", "content": ["id_rj_profile", CH, 1],
                 "options": [["Public", 0], ["Personal", 1], ["Gaming", 2]]},
                {"label": "Public colour", "type": "color", "content": ["id_rj_color_pub", CH, 7]},
                {"label": "Personal colour", "type": "color", "content": ["id_rj_color_per", CH, 8]},
                {"label": "Gaming colour", "type": "color", "content": ["id_rj_color_gam", CH, 9]},
                {"label": "Profile colour on Delete", "type": "dropdown", "content": ["id_rj_del_mode", CH, 2],
                 "options": [["While FN held / 2s after switching", 0], ["Always", 1]]},
            ],
        },
        {
            "label": "Gaming (SOCD)",
            "content": [
                {"label": "WASD SOCD mode (Gaming profile)", "type": "dropdown", "content": ["id_rj_socd_mode", CH, 3],
                 "options": [["Last input wins", 0], ["Neutral (cancel)", 1], ["First input wins", 2], ["Off", 3]]},
            ],
        },
        {
            "label": "Startup Animation",
            "content": [
                {"label": "Animation (plays on power-up)", "type": "dropdown", "content": ["id_rj_startup", CH, 4],
                 "options": [["Off", 0], ["Welcome", 9], ["Ignition", 6], ["Sunrise", 11], ["Comets", 1],
                             ["Curtain", 5], ["Rainbow Wipe", 2], ["Larson Scanner", 3], ["Meteor", 4], ["Fire", 7],
                             ["Power Up", 8], ["Ripple", 10], ["Bounce", 12], ["Heartbeat", 13],
                             ["Shuffle (new one every power-up)", 14]]},
                {"showIf": "{id_rj_startup} != 0", "label": "Colour", "type": "dropdown",
                 "content": ["id_rj_anim_color", CH, 10],
                 "options": [["Auto (animation's own)", 0], ["Profile colour", 1], ["Custom", 2], ["Rainbow", 3]]},
                {"showIf": "{id_rj_startup} != 0 && {id_rj_anim_color} == 2", "label": "Custom colour",
                 "type": "color", "content": ["id_rj_anim_custom", CH, 11]},
                {"label": "Preview", "type": "button", "options": [1], "content": ["id_rj_anim_play", CH, 12]},
            ],
        },
        {
            "label": "Light Behaviour",
            "content": [
                {"label": "Effect brightness while FN is held (%)", "type": "range", "options": [0, 100],
                 "content": ["id_rj_fn_dim_pct", CH, 21]},
                {"label": "Dim when idle", "type": "toggle", "content": ["id_rj_idle_dim", CH, 13]},
                {"showIf": "{id_rj_idle_dim} == 1", "label": "Idle brightness (%)", "type": "range", "options": [5, 95],
                 "content": ["id_rj_idle_dim_pct", CH, 14]},
                {"showIf": "{id_rj_idle_dim} == 1", "label": "Dim after (seconds)", "type": "range", "options": [5, 50],
                 "content": ["id_rj_idle_dim_s", CH, 15]},
                {"label": "Fade out before sleep / timeout", "type": "toggle", "content": ["id_rj_sleep_fade", CH, 19]},
                {"label": "Brightness / colour HUD on F1-F12", "type": "toggle", "content": ["id_rj_hud", CH, 16]},
                {"label": "Colour sweep on profile switch", "type": "toggle", "content": ["id_rj_sweep", CH, 17]},
                {"label": "Pulse when a connection comes up", "type": "toggle", "content": ["id_rj_conn_pulse", CH, 18]},
            ],
        },
        {
            "label": "Navigation (FN + HJKL)",
            "content": [
                {"label": "Hide FN lights while navigating", "type": "toggle", "content": ["id_rj_nav_focus", CH, 22]},
                {"showIf": "{id_rj_nav_focus} == 1", "label": "Effect while navigating", "type": "dropdown",
                 "content": ["id_rj_nav_fx", CH, 23], "options": [["Dimmed (like FN)", 0], ["Normal", 1], ["Off", 2]]},
                {"showIf": "{id_rj_nav_focus} == 1", "label": "Show the pressed arrow on F5-F8", "type": "toggle",
                 "content": ["id_rj_nav_echo", CH, 24]},
            ],
        },
        {
            "label": "Lights & Power",
            "content": [
                {"label": "Effect lights on", "type": "toggle", "content": ["id_rj_fx_on", CH, 5]},
                {"label": "Hard RGB off after idle (minutes, 0 = never)", "type": "range", "options": [0, 240],
                 "content": ["id_rj_rgb_off_min", CH, 6]},
                {"label": "Keep keys typed while wireless reconnects after sleep", "type": "toggle",
                 "content": ["id_rj_wake_replay", CH, 20]},
            ],
        },
    ],
}

via = {
    "name": "Bridge75: Jscio Edition",
    "vendorId": kb["usb"]["vid"],
    "productId": kb["usb"]["pid"],
    "matrix": {"rows": 6, "cols": 15},
    "keycodes": ["qmk_rgb_matrix_keycodes"],
    "menus": [lighting, settings],
    "customKeycodes": custom_keycodes,
    "layouts": {"keymap": kle},
}

with open(out, "w", newline="\n") as f:
    json.dump(via, f, indent=2)
    f.write("\n")

labels = [k for r in kle for k in r if isinstance(k, str)]
print("keys:", len(labels), "unique:", len(set(labels)), "layout keys:", len(info_layout))
