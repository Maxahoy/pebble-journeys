#!/usr/bin/env python3
"""
WCAG contrast ratio checker for Pebble Journeys theme colors.

Pebble uses 6-bit color (2 bits per R/G/B channel).
Argb byte format: 11_RR_GG_BB
Channel values: 0→0, 1→85, 2→170, 3→255
"""

SCALE = [0, 85, 170, 255]

def to_rgb(argb):
    return (SCALE[(argb >> 4) & 3], SCALE[(argb >> 2) & 3], SCALE[argb & 3])

def luminance(argb):
    def lin(c):
        s = c / 255.0
        return s / 12.92 if s <= 0.04045 else ((s + 0.055) / 1.055) ** 2.4
    r, g, b = to_rgb(argb)
    return 0.2126 * lin(r) + 0.7152 * lin(g) + 0.0722 * lin(b)

def contrast(fg, bg):
    l1, l2 = luminance(fg), luminance(bg)
    hi, lo = max(l1, l2), min(l1, l2)
    return (hi + 0.05) / (lo + 0.05)

def hexc(argb):
    r, g, b = to_rgb(argb)
    return f"#{r:02X}{g:02X}{b:02X}"

# Mirrors s_themes[] in main.c — update both together
THEMES = [
    {
        "name": "0: Vaporwave",
        "sky":    [0b11000001, 0b11010001, 0b11100010, 0b11100011,
                   0b11110011, 0b11110111, 0b11110111],
        "road":   0b11000001, "grid_v": 0b11001111, "grid_h1": 0b11001111, "grid_h2": 0b11110011,
        "bar_bg": 0b11000001, "txt_ovr": 0b11001111, "txt_day": 0b11110111,
        "info1_bg": 0b11000001, "info2_bg": 0b11010001,
        "txt_time": 0b11111111, "txt_leg": 0b11001111,
    },
    {
        "name": "1: Beach",
        "sky":    [0b11000011, 0b11000111, 0b11001111, 0b11001110,
                   0b11111001, 0b11111000, 0b11111000],
        "road":   0b11111001, "grid_v": 0b11000101, "grid_h1": 0b11000101, "grid_h2": 0b11000101,
        "bar_bg": 0b11000101, "txt_ovr": 0b11111111, "txt_day": 0b11001111,
        "info1_bg": 0b11000101, "info2_bg": 0b11000111,
        "txt_time": 0b11000000, "txt_leg": 0b11111111,
    },
    {
        "name": "2: Mountain",
        "sky":    [0b11000001, 0b11010010, 0b11100111, 0b11110011,
                   0b11110000, 0b11100100, 0b11100100],
        "road":   0b11010101, "grid_v": 0b11111111, "grid_h1": 0b11111111, "grid_h2": 0b11101010,
        "bar_bg": 0b11010101, "txt_ovr": 0b11111000, "txt_day": 0b11001110,
        "info1_bg": 0b11010101, "info2_bg": 0b11000100,
        "txt_time": 0b11111111, "txt_leg": 0b11111110,
    },
    {
        "name": "3: Winter",
        "sky":    [0b11000001, 0b11000010, 0b11000111, 0b11010111,
                   0b11101010, 0b11111110, 0b11111111],
        "road":   0b11010101, "grid_v": 0b11111111, "grid_h1": 0b11111111, "grid_h2": 0b11001111,
        "bar_bg": 0b11010101, "txt_ovr": 0b11001111, "txt_day": 0b11111110,
        "info1_bg": 0b11000001, "info2_bg": 0b11010101,
        "txt_time": 0b11111111, "txt_leg": 0b11001111,
    },
    {
        "name": "4: City",
        "sky":    [0b11000000, 0b11010000, 0b11100001, 0b11100100,
                   0b11110100, 0b11110100, 0b11110100],
        "road":   0b11010000, "grid_v": 0b11111000, "grid_h1": 0b11111000, "grid_h2": 0b11110100,
        "bar_bg": 0b11000000, "txt_ovr": 0b11111000, "txt_day": 0b11110100,
        "info1_bg": 0b11000000, "info2_bg": 0b11000000,
        "txt_time": 0b11111111, "txt_leg": 0b11111000,
    },
    {
        "name": "5: Plains",
        "sky":    [0b11000011, 0b11000111, 0b11001111, 0b11011011,
                   0b11111111, 0b11111101, 0b11111000],
        "road":   0b11001001, "grid_v": 0b11000001, "grid_h1": 0b11000001, "grid_h2": 0b11010000,
        "bar_bg": 0b11000001, "txt_ovr": 0b11111111, "txt_day": 0b11001111,
        "info1_bg": 0b11000001, "info2_bg": 0b11000001,
        "txt_time": 0b11000001, "txt_leg": 0b11111111,
    },
    {
        "name": "6: Desert",
        "sky":    [0b11001011, 0b11001110, 0b11111110, 0b11111110,
                   0b11111001, 0b11111000, 0b11110100],
        "road":   0b11111001, "grid_v": 0b11100000, "grid_h1": 0b11100000, "grid_h2": 0b11100000,
        "bar_bg": 0b11010000, "txt_ovr": 0b11111111, "txt_day": 0b11001111,
        "info1_bg": 0b11010000, "info2_bg": 0b11010000,
        "txt_time": 0b11000001, "txt_leg": 0b11000001,
    },
]

# Sky band Y ranges: {14,28,44,60,76,88,97}
# txt_leg TextLayer: Y=3–23 → sky[0] (Y=0–14) and sky[1] (Y=14–28)
# txt_time TextLayer: Y=48–95 → sky[3] (44–60), sky[4] (60–76), sky[5] (76–88), sky[6] (88–97)

def check_theme(t):
    pairs = [
        ("txt_leg  / sky[0]",   t["txt_leg"],  t["sky"][0]),
        ("txt_leg  / sky[1]",   t["txt_leg"],  t["sky"][1]),
        ("txt_time / sky[3]",   t["txt_time"], t["sky"][3]),
        ("txt_time / sky[4]",   t["txt_time"], t["sky"][4]),
        ("txt_time / sky[5]",   t["txt_time"], t["sky"][5]),
        ("txt_time / sky[6]",   t["txt_time"], t["sky"][6]),
        ("txt_ovr  / bar_bg",   t["txt_ovr"],  t["bar_bg"]),
        ("txt_ovr  / info1_bg", t["txt_ovr"],  t["info1_bg"]),
        ("txt_day  / info1_bg", t["txt_day"],  t["info1_bg"]),
        ("txt_day  / info2_bg", t["txt_day"],  t["info2_bg"]),
        ("grid_v   / road",     t["grid_v"],   t["road"]),
        ("grid_h1  / road",     t["grid_h1"],  t["road"]),
        ("grid_h2  / road",     t["grid_h2"],  t["road"]),
    ]
    print(f"\n{'='*60}")
    print(f"  {t['name']}")
    print(f"{'='*60}")
    fails = 0
    for label, fg, bg in pairs:
        ratio = contrast(fg, bg)
        # 3.0:1 minimum for large UI text/graphics; 4.5:1 for small text
        ok = ratio >= 3.0
        if not ok:
            fails += 1
        status = "  OK " if ok else " FAIL"
        print(f"  {status}  {label:26s}  {ratio:5.1f}:1   {hexc(fg)} on {hexc(bg)}")
    if fails == 0:
        print("  All pairs pass 3:1")
    return fails

total_fails = 0
for theme in THEMES:
    total_fails += check_theme(theme)

print(f"\n{'='*60}")
print(f"  Total failing pairs: {total_fails}")
print(f"{'='*60}")
