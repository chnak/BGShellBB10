# -*- coding: utf-8 -*-
"""Redraw the app icon: dark tile, gradient-green prompt chevron + block cursor.

Matches the in-app chrome (background #0b0e11, accent #00e05a), so icon and UI
read as one product.  Drawn at 4x and downsampled for clean edges.

    python3 tools/make_icon.py

Writes assets/BGshellPlus_icon{144,110,96,90}.png (the names bar-descriptor.xml
already points at) and a 512px preview under tools/.
"""
import os
import shutil

from PIL import Image, ImageDraw, ImageFilter

BASE = 144
SS = 4
S = BASE * SS

GREEN_TOP = (0x3c, 0xff, 0x88)
GREEN_BOT = (0x00, 0xbe, 0x4b)
BG_TOP = (0x18, 0x20, 0x29)
BG_BOT = (0x0a, 0x0d, 0x10)
BORDER = (0x2b, 0x35, 0x40)
DIM = (0x27, 0x34, 0x3e)


def lerp(a, b, t):
    return tuple(int(round(a[i] + (b[i] - a[i]) * t)) for i in range(3))


def vertical_gradient(top, bot, size, gamma=1.0):
    grad = Image.new('RGB', (1, size))
    px = grad.load()
    for y in range(size):
        t = (y / float(size - 1)) ** gamma
        px[0, y] = lerp(top, bot, t)
    return grad.resize((size, size))


# ---------------------------------------------------------------- backdrop --
bg = vertical_gradient(BG_TOP, BG_BOT, S, gamma=0.85)
radius = int(S * 0.18)
# BB10 crops app icons to a rounded square itself, so the pixels stay opaque
# (like the original icon); the rounded outline is drawn inside instead.
img = Image.new('RGBA', (S, S), (0, 0, 0, 255))
img.paste(bg, (0, 0))

border_inset = int(S * 0.045)
d = ImageDraw.Draw(img)
d.rounded_rectangle([border_inset, border_inset, S - 1 - border_inset, S - 1 - border_inset],
                    radius=radius - border_inset, outline=BORDER + (255,), width=SS * 2)

# --------------------------------------------------------------- glyphs ----
stroke = int(S * 0.088)          # ~12.7px at 144
cx0, cy0 = int(S * 0.280), int(S * 0.310)
cx1, cy1 = int(S * 0.500), int(S * 0.495)
cx2, cy2 = int(S * 0.280), int(S * 0.680)

bx0, by0 = int(S * 0.555), int(S * 0.500)
bx1, by1 = int(S * 0.735), int(S * 0.680)

out_lines = [((int(S * 0.280), int(S * 0.790)), (int(S * 0.735), int(S * 0.790))),
             ((int(S * 0.280), int(S * 0.875)), (int(S * 0.545), int(S * 0.875)))]


def glyph_mask():
    """White mask of the chevron + block cursor."""
    m = Image.new('L', (S, S), 0)
    dd = ImageDraw.Draw(m)
    dd.line([(cx0, cy0), (cx1, cy1)], fill=255, width=stroke)
    dd.line([(cx2, cy2), (cx1, cy1)], fill=255, width=stroke)
    for (x, y) in [(cx0, cy0), (cx1, cy1), (cx2, cy2)]:
        r = stroke // 2
        dd.ellipse([x - r, y - r, x + r, y + r], fill=255)
    dd.rounded_rectangle([bx0, by0, bx1, by1], radius=int(S * 0.018), fill=255)
    return m


mask_glyph = glyph_mask()
green = vertical_gradient(GREEN_TOP, GREEN_BOT, S, gamma=0.9)

# soft glow underneath
glow = Image.new('RGBA', (S, S), (0, 0, 0, 0))
glow.paste(Image.new('RGBA', (S, S), GREEN_BOT + (255,)), (0, 0), mask_glyph)
glow = glow.filter(ImageFilter.GaussianBlur(S * 0.022))
glow.putalpha(glow.split()[3].point(lambda v: int(v * 0.50)))
img = Image.alpha_composite(img, glow)

# crisp gradient-green glyph on top
accent = Image.new('RGBA', (S, S), (0, 0, 0, 0))
accent.paste(green, (0, 0), mask_glyph)
img = Image.alpha_composite(img, accent)

# dim "output" lines under the prompt
d = ImageDraw.Draw(img)
for a, b in out_lines:
    d.line([a, b], fill=DIM + (255,), width=SS * 3)
    for (x, y) in [a, b]:
        d.ellipse([x - SS * 1.5, y - SS * 1.5, x + SS * 1.5, y + SS * 1.5],
                  fill=DIM + (255,))

# ------------------------------------------------------------------ save ---
targets = [('assets/BGshellPlus_icon144x144.png', 144),
           ('assets/BGshellPlus_icon110x110.png', 110),
           ('assets/BGshellPlus_icon96x96.png', 96),
           ('assets/BGshellPlus_icon90x90.png', 90)]

for path, size in targets:
    if os.path.exists(path) and not os.path.exists(path + '.orig'):
        shutil.copy2(path, path + '.orig')
    out = img if size == BASE else img.resize((size, size), Image.LANCZOS)
    out.save(path)
    print('wrote %s (%dx%d)' % (path, size, size))

# 512px preview, kept out of assets/ so it is not packaged into the .bar
img.resize((512, 512), Image.LANCZOS).save('tools/icon_preview_512.png')
print('wrote tools/icon_preview_512.png')
